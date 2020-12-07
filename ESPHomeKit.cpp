/**
 * @file ESPHomeKit.cpp
 * @author Max Vissing (max_vissing@yahoo.de)
 * @brief 
 * @version 0.1
 * @date 2020-05-25
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "ESPHomeKit.h"
#include "crypto/srp.h"

/**
 * @brief Construct a new ESPHomeKit::ESPHomeKit object
 * 
 */
ESPHomeKit::ESPHomeKit() : server(WiFiServer(PORT)), accessory(nullptr), configNumber(1) {
    HKStorage::checkStorage();
}

/**
 * @brief Destroy the ESPHomeKit::ESPHomeKit object
 * 
 */
ESPHomeKit::~ESPHomeKit() {
}

/**
 * @brief Call it in the setup routine after you have set your accessory. Sets up the accessory and the server.
 * 
 */
void ESPHomeKit::setup(HKAccessory *accessory) {
    this->accessory = accessory;

    HKLOGINFO("[ESPHomeKit::setup] AccessoryID: %s\r\n", HKStorage::getAccessoryId().c_str());

    #if HKLOGLEVEL <= 1
    for (auto pairing : HKStorage::getPairings()) {
        HKLOGINFO("[HKStorage] Pairing id=%i deviceId=%36s permissions=%i\r\n", pairing->id, pairing->deviceId, pairing->permissions);
    }
    #endif

    HKLOGINFO("[ESPHomeKit::setup] Password: %s\r\n", HKPASSWORD);

    this->accessory->prepareIDs();
}

/**
 * @brief Starts the server
 * 
 */
void ESPHomeKit::begin() {
    if (!HKStorage::isPaired()) {
        srp = new Srp(String(HKPASSWORD).c_str());
    }
    setupMDNS();
    server.begin();
}

/**
 * @brief Call it in the update routine to accept new connections and handling the accessory.
 * 
 */
void ESPHomeKit::update() {
    MDNS.update();
    handleClient();
    accessory->run();
}

/**
 * @brief Look for clients that are in pairing process
 * 
 * @return true A client is pairing, busy
 * @return false Free to pair
 */
bool ESPHomeKit::isPairing() {
    for (HKClient *client : clients) {
        if (client->isPairing()) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Get saved name, name from info service or generate name
 * 
 * @return String name
 */
String ESPHomeKit::getName() {
    if (!accessory) {
        HKLOGERROR("No Accessory in HomeKit");
        return "";
    }

    HKService *info = accessory->getService(HKServiceAccessoryInfo);
    if (!info) {
        HKLOGERROR("No Accessory Information Service in Accessory");
    }

    HKCharacteristic *serviceName = info->getCharacteristic(HKCharacteristicName);
    if (!serviceName) {
        HKLOGERROR("No Accessory Name in Accessory Information Service");
    }

    String name = serviceName->getValue().stringValue;
    #ifndef HK_UNIQUE_NAME
    String accessoryId = HKStorage::getAccessoryId();
    name += "-" + accessoryId.substring(0, 2) + accessoryId.substring(3, 5);
    #endif
    return name;
}

/**
 * @brief Get accessory
 * 
 * @return HKAccessory* accessory
 */
HKAccessory *ESPHomeKit::getAccessory() {
    return accessory;
}

/**
 * @brief Reset EEPROM
 * 
 */
void ESPHomeKit::reset() {
    HKStorage::reset();
}

/**
 * @brief Reset all Pairings and keep WiFi
 * 
 */
void ESPHomeKit::resetPairings() {
    HKStorage::resetPairings();
}

bool ESPHomeKit::setupMDNS() {
    HKLOGINFO("[ESPHomeKit::setupMDNS] Setup mDNS\r\n");
    String uniqueName = getName();
    if (!mdnsService && !MDNS.begin(uniqueName)) {
        HKLOGERROR("[ESPHomeKit::setupMDNS] Failed to begin mDNS\r\n");
        return false;
    }
    
    const char service[] = "hap";
    const char protocol[] = "tcp";
    MDNS.setInstanceName(uniqueName);
    if (!mdnsService) {
        mdnsService = MDNS.addService(nullptr, service, protocol, PORT);
        if (!mdnsService) {
            HKLOGERROR("[ESPHomeKit::setupMDNS] Failed to add service\r\n");
            return false;
        }
    }

    HKService *info = getAccessory()->getService(HKServiceAccessoryInfo);
    if (info == nullptr) {
        return false;
    }
    HKCharacteristic *model = info->getCharacteristic(HKCharacteristicModelName);
    if (model == nullptr) {
        return false;
    }

    if (!MDNS.addServiceTxt(service, protocol, "md", model->getValue().stringValue)) {
        HKLOGERROR("[ESPHomeKit::setupMDNS] Failed to add md model\r\n");
        return false;
    }
    if (!MDNS.addServiceTxt(service, protocol, "pv", "1.0")) {
        HKLOGERROR("[ESPHomeKit::setupMDNS] Failed to add pv \"1.0\"\r\n");
        return false;
    }
    if (!MDNS.addServiceTxt(service, protocol, "id", HKStorage::getAccessoryId())) {
        HKLOGERROR("[ESPHomeKit::setupMDNS] Failed to add id accessoryId\r\n");
        return false;
    }
    if (!MDNS.addServiceTxt(service, protocol, "c#", String(configNumber))) {
        HKLOGERROR("[ESPHomeKit::setupMDNS] Failed to add c# configNumber\r\n");
        return false;
    }
    if (!MDNS.addServiceTxt(service, protocol, "s#", "1")) {  // State number
        HKLOGERROR("[ESPHomeKit::setupMDNS] Failed to add s#\r\n");
        return false;
    }
    if (!MDNS.addServiceTxt(service, protocol, "ff", "0")) {  // feature flags
        //   bit 0 - supports HAP pairing. required for all ESPHomeKit accessories
        //   bits 1-7 - reserved
        HKLOGERROR("[ESPHomeKit::setupMDNS] Failed to add ff\r\n");
        return false;
    }
    if (!MDNS.addServiceTxt(service, protocol, "sf", String(HKStorage::isPaired() ? 0 : 1))) {  // status flags
        //   bit 0 - not paired
        //   bit 1 - not configured to join WiFi
        //   bit 2 - problem detected on accessory
        //   bits 3-7 - reserved
        HKLOGERROR("[ESPHomeKit::setupMDNS] Failed to add sf paired\r\n");
        return false;
    }
    if (!MDNS.addServiceTxt(service, protocol, "ci", String(accessory->getCategory()))) {
        HKLOGERROR("[ESPHomeKit::setupMDNS] Failed to add ci category\r\n");
        return false;
    }

    #ifdef HKSETUPID
    String setupId = HKSETUPID;
    if (setupId.length() == 4) {
        size_t dataSize = setupId.length() + HKStorage::getAccessoryId().length() + 1;
        char *data = (char *) malloc(dataSize);
        snprintf(data, dataSize, "%s%s", setupId.c_str(), HKStorage::getAccessoryId().c_str());
        data[dataSize-1] = 0;

        unsigned char shaHash[64];
        SHA512 sha512 = SHA512();
        sha512.reset();
        sha512.update(data, 21);
        free(data);
        sha512.finalize(shaHash, 64);

        String encoded = base64::encode(shaHash, 4, false);

        if (!MDNS.addServiceTxt(service, protocol, "sh", encoded)) {
            HKLOGERROR("[ESPHomeKit::setupMDNS] Failed to add ci category\r\n");
            return false;
        }
    }
    #endif

    return true;
}

/**
 * @brief look for new clients and handle received messages
 * 
 */
void ESPHomeKit::handleClient() {
    WiFiClient newClient = server.available();
    if (newClient && clients.size() < MAX_CLIENTS) {
        HKLOGINFO("[ESPHomeKit::handleClient] New Client connected (%s:%d)\r\n", newClient.remoteIP().toString().c_str(), newClient.remotePort());
        newClient.setTimeout(10000);
        newClient.keepAlive(180,30, 4);
        clients.push_back(new HKClient(newClient));
    }

    for (auto it = clients.begin(); it != clients.end();) {
        size_t messageSize = (*it)->available();
        if (messageSize) {
            messageSize = (*it)->getMessageSize(messageSize);
            uint8_t *message = (uint8_t *) malloc(messageSize);
            (*it)->receive(message, messageSize);
            parseMessage(*it, message, messageSize);
            free(message);
        } else {
            (*it)->processNotifications();
        }

        if (!(*it)->isConnected()) {
            HKLOGINFO("[ESPHomeKit::handleClient] Client disconnected\r\n");
            accessory->clearCallbackEvents(*it);
            delete *it;
            it = clients.erase(it);
        } else {
            it++;
        }
    }
}

/**
 * @brief parse given message and execute HomeKit endpoints
 * 
 * @param client active client who sent message
 * @param message the sent message in bytes
 * @param messageSize size of message
 */
void ESPHomeKit::parseMessage(HKClient *client, uint8_t *message, const size_t &messageSize) {
    #if HKLOGLEVEL == 0
    HKLOGDEBUGSINGLE("------------- Received -------------\r\n");
    for (size_t i = 0; i < messageSize; i++) {
        byte item = *(message + i);
        if ((item >= ' ' && item <= '}' && item != '\\')) {
            HKLOGDEBUGSINGLE("%c", item);
        } else if (item == '\r') {
            HKLOGDEBUGSINGLE("\\r");  
        } else if (item == '\n') {
            HKLOGDEBUGSINGLE("\\n\n");  
        } else if (item < 0x10) {
            HKLOGDEBUGSINGLE("\\x0%x", item);
        } else {
            HKLOGDEBUGSINGLE("\\x%x", item);
        }
    }
    HKLOGDEBUGSINGLE("\r\n----------- End Received -----------\r\n");
    #endif

    size_t linePos = 0;
    for (; linePos < messageSize && message[linePos] != '\r'; linePos++) {  }
    if (linePos == messageSize) return;
    message[linePos] = 0;
    linePos += 2;
    String line = String((char *)message);

    int prevPos = 0;
    int pos = 0;
    pos = line.indexOf(' ', prevPos);
    String method = line.substring(prevPos, pos);
    prevPos = pos+1;

    pos = line.indexOf(' ', prevPos);
    String url = line.substring(prevPos, pos);

    String searchStr = "";
    int hasSearch = url.indexOf('?');
    if (hasSearch != -1) {
        searchStr = url.substring(hasSearch + 1);
        url = url.substring(0, hasSearch);
    }

    HKLOGDEBUG("[ESPHomeKit::parseMessage] url: %s searchStr: %s\r\n", url.c_str(), searchStr.c_str());

    if (method == "GET") {
        if (url == "/accessories") {
            onGetAccessories(client);
            return;
        } else if (url == "/characteristics") {
            String id;
            int searchPos = searchStr.indexOf("id");
            if (searchPos != -1) {
                int end = searchStr.indexOf('&', searchPos);
                if (end == -1) {
                    end = searchStr.length();
                }
                id = searchStr.substring(searchPos+3, end);
            }
            bool meta = false;
            searchPos = searchStr.indexOf("meta");
            if (searchPos != -1) {
                int end = searchStr.indexOf('&', searchPos);
                if (end == -1) {
                    end = searchStr.length();
                }
                meta = searchStr.substring(searchPos+5, end) == "1";
            }
            bool perms = false;
            searchPos = searchStr.indexOf("perms");
            if (searchPos != -1) {
                int end = searchStr.indexOf('&', searchPos);
                if (end == -1) {
                    end = searchStr.length();
                }
                perms = searchStr.substring(searchPos+6, end) == "1";
            }
            bool type = false;
            searchPos = searchStr.indexOf("type");
            if (searchPos != -1) {
                int end = searchStr.indexOf('&', searchPos);
                if (end == -1) {
                    end = searchStr.length();
                }
                type = searchStr.substring(searchPos+5, end) == "1";
            }
            bool ev = false;
            searchPos = searchStr.indexOf("ev");
            if (searchPos != -1) {
                int end = searchStr.indexOf('&', searchPos);
                if (end == -1) {
                    end = searchStr.length();
                }
                ev = searchStr.substring(searchPos+3, end) == "1";
            }
            onGetCharacteristics(client, id, meta, perms, type, ev);
            return;
        }
    } else if (method == "POST" && url == "/identify") {
        onIdentify(client);
        return;
    } else {
        String contentType;
        uint32_t contentLength = 0;
        while (linePos < messageSize) {
            int lineBegin = linePos;
            for (; linePos < messageSize && message[linePos] != '\r'; linePos++) {  }
            if (linePos == messageSize) {
                linePos = lineBegin;
                break;
            }
            message[linePos] = 0;
            linePos += 2;
            String line = String((char *) message + lineBegin);

            if (line == "") {
                break;
            }

            int headerDiv = line.indexOf(':');
            if (headerDiv == -1) {
                break;
            }
            String headerName = line.substring(0, headerDiv);
            String headerValue = line.substring(headerDiv + 2);
            if (headerName.equalsIgnoreCase("Content-Type")) {
                contentType = headerValue;
            } else if (headerName.equalsIgnoreCase("Content-Length")) {
                contentLength = headerValue.toInt();
            }
        }

        uint8_t *buffer = (uint8_t *) malloc(contentLength + 1);
        buffer[contentLength] = 0;
        size_t tempSize = messageSize - linePos;
        memcpy(buffer, message + linePos, tempSize);

        if (!client->isEncrypted() && tempSize < contentLength) {
            if (!client->readBytesWithTimeout(buffer + tempSize, contentLength - tempSize, 2000)) {
                free(buffer);
                HKLOGWARNING("[ESPHomeKit::parseMessage] Could not receive complete message: timeout\r\n");
                return;
            }
        }

        if (method == "PUT" && url == "/characteristics") {
            onUpdateCharacteristics(client, buffer, contentLength);
            free(buffer);
            return;
        } else if (method == "POST") {
            if (url == "/pair-setup") {
                onPairSetup(client, buffer, contentLength);
                free(buffer);
                return;
            } else if (url == "/pair-verify") {
                onPairVerify(client, buffer, contentLength);
                free(buffer);
                return;
            } else if (url == "/pairings") {
                onPairings(client, buffer, contentLength);
                free(buffer);
                return;
            }
        }
    }
    // handle 404
}
