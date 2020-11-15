//
// Created by Max Vissing on 2019-04-26.
//

#define LWIP_INTERNAL

#include "HKServer.h"
#include "lwip/tcp.h"
#include <include/ClientContext.h>
#include <base64.h>

#define NOTIFICATION_UPDATE_FREQUENCY 1000

HKServer::HKServer(ESPHomeKit *hk) : WiFiServer(PORT), hk(hk), mdnsService() {
}

void HKServer::setup() {
    setupMDNS();

    begin();
}

void HKServer::update() {
    MDNS.update();

    HKClient *newClient = availableHK();
    if (newClient && *newClient) {
        if (clients.size() >= MAX_CLIENTS) {
            return;
        }

        HKLOGINFO("[HKServer::update] New Client connected (%s:%d)\r\n", newClient->remoteIP().toString().c_str(), newClient->remotePort());

        newClient->setTimeout(10000);
        newClient->keepAlive(180, 30, 4);
        clients.push_back(newClient);
    } else {
        delete newClient;
    }

    for (auto it = clients.begin(); it != clients.end();) {
        auto client = *it;
        if (client->available()) {
            client->received();
        } else {
            processNotificationsFromClient(client);
        }

        if (!client->connected()) {
            HKLOGINFO("[HKServer::update] Client disconnected\r\n");
            hk->getAccessory()->clearCallbackEvents(client);
            delete *it;
            it = clients.erase(it);
        } else {
            it++;
        }
    }
}

bool HKServer::isPairing() {
    for (auto client : clients) {
        if (client->pairing) {
            return true;
        }
    }
    return false;
}

HKClient *HKServer::availableHK(byte* status) {
    (void) status;
    if (_unclaimed) {
        auto result = new HKClient(this, _unclaimed);
        _unclaimed = _unclaimed->next();
        result->setNoDelay(getNoDelay());
        DEBUGV("WS:av\r\n");
        return result;
    }

    optimistic_yield(1000);
    return nullptr;
}

int HKServer::setupMDNS() {
    HKLOGINFO("[HKServer::setupMDNS] Setup mDNS\r\n");
    String uniqueName = hk->getName();

    if (!mdnsService && !MDNS.begin(uniqueName)) {
        HKLOGERROR("[HKServer::setupMDNS] Failed to begin mDNS\r\n");
        return -1;
    }
    
    const char *service = "hap";
    const char *protocol = "tcp";
    MDNS.setInstanceName(uniqueName);
    if (!mdnsService) {
        mdnsService = MDNS.addService(nullptr, service, protocol, PORT);
        if (!mdnsService) {
            HKLOGERROR("[HKServer::setupMDNS] Failed to add service\r\n");
            return -1;
        }
    }

    HKService *info = hk->getAccessory()->getService(HKServiceAccessoryInfo);
    if (info == nullptr) {
        return -1;
    }
    HKCharacteristic *model = info->getCharacteristic(HKCharacteristicModelName);
    if (model == nullptr) {
        return -1;
    }

    if (!MDNS.addServiceTxt(service, protocol, "md", model->getValue().stringValue)) {
        HKLOGERROR("[HKServer::setupMDNS] Failed to add md model\r\n");
        return false;
    }
    if (!MDNS.addServiceTxt(service, protocol, "pv", "1.0")) {
        HKLOGERROR("[HKServer::setupMDNS] Failed to add pv \"1.0\"\r\n");
        return false;
    }
    if (!MDNS.addServiceTxt(service, protocol, "id", hk->getAccessoryId())) {
        HKLOGERROR("[HKServer::setupMDNS] Failed to add id accessoryId\r\n");
        return false;
    }
    if (!MDNS.addServiceTxt(service, protocol, "c#", String(hk->getConfigNumber()))) {
        HKLOGERROR("[HKServer::setupMDNS] Failed to add c# configNumber\r\n");
        return false;
    }
    if (!MDNS.addServiceTxt(service, protocol, "s#", "1")) {  // State number
        HKLOGERROR("[HKServer::setupMDNS] Failed to add s#\r\n");
        return false;
    }
    if (!MDNS.addServiceTxt(service, protocol, "ff", "0")) {  // feature flags
        //   bit 0 - supports HAP pairing. required for all ESPHomeKit accessories
        //   bits 1-7 - reserved
        HKLOGERROR("[HKServer::setupMDNS] Failed to add ff\r\n");
        return false;
    }
    if (!MDNS.addServiceTxt(service, protocol, "sf", String(hk->getStorage()->isPaired() ? 0 : 1))) {  // status flags
        //   bit 0 - not paired
        //   bit 1 - not configured to join WiFi
        //   bit 2 - problem detected on accessory
        //   bits 3-7 - reserved
        HKLOGERROR("[HKServer::setupMDNS] Failed to add sf paired\r\n");
        return false;
    }
    if (!MDNS.addServiceTxt(service, protocol, "ci", String(hk->getAccessory()->getCategory()))) {
        HKLOGERROR("[HKServer::setupMDNS] Failed to add ci category\r\n");
        return false;
    }

#ifdef HKSETUPID
        char *data = (char *) malloc(22);
        snprintf(data, 22, "%s%s", HKSETUPID, hk->getAccessoryId().c_str());
        data[21] = 0;

        unsigned char shaHash[64];
        SHA512 sha512 = SHA512();
        sha512.reset();
        sha512.update(data, 21);
        sha512.finalize(shaHash, 64);

        free(data);

        String encoded = base64::encode(shaHash, 4, false);

        if (!MDNS.addServiceTxt(service, protocol, "sh", encoded)) {
            HKLOGERROR("[HKServer::setupMDNS] Failed to add ci category\r\n");
            return false;
        }
#endif

    return true;
}

void HKServer::processNotificationsFromClient(HKClient *client) {
    if (millis() - client->lastUpdate > NOTIFICATION_UPDATE_FREQUENCY && !client->events.empty()) {
        auto it = client->events.begin();

        auto eventsHead = (ClientEvent *) malloc(sizeof(ClientEvent));
        eventsHead->characteristic = (*it)->getCharacteristic();
        eventsHead->value = (*it)->getValue();
        eventsHead->next = nullptr;

        delete *it;
        it = client->events.erase(it);

        ClientEvent *eventsTail = eventsHead;
        for (; it != client->events.end();) {
            ClientEvent *e = eventsHead;
            while (e) {
                if (e->characteristic == (*it)->getCharacteristic()) {
                    break;
                }
                e = e->next;
            }

            if (!e) {
                e = (ClientEvent *) malloc(sizeof(ClientEvent));
                e->characteristic = (*it)->getCharacteristic();
                e->next = nullptr;

                eventsTail->next = e;
                eventsTail = e;
            }

            e->value = (*it)->getValue();

            delete *it;
            it = client->events.erase(it);
        }

        client->sendEvents(eventsHead);

        ClientEvent *e = eventsHead;
        while (e) {
            ClientEvent *next = e->next;
            free(e);

            e = next;
        }

        client->lastUpdate = millis();
    }
}
