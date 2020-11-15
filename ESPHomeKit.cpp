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
ESPHomeKit::ESPHomeKit() : storage(new HKStorage()), server(new HKServer(this)), accessory(nullptr), configNumber(1) {
}

/**
 * @brief Destroy the ESPHomeKit::ESPHomeKit object
 * 
 */
ESPHomeKit::~ESPHomeKit() {
    delete server;
    delete storage;
}

/**
 * @brief Call it in the setup routine after you have set your accessory. Sets up the accessory and the server.
 * 
 */
void ESPHomeKit::setup() {
    HKLOGINFO("[ESPHomeKit::setup] AccessoryID: %s\r\n", storage->getAccessoryId().c_str());

#if HKLOGLEVEL <= 1
    for (auto pairing : storage->getPairings()) {
        HKLOGINFO("[HKStorage] Pairing id=%i deviceId=%36s permissions=%i\r\n", pairing->id, pairing->deviceId, pairing->permissions);
    }
#endif

    srp_init();

    HKLOGINFO("[ESPHomeKit::setup] Password: %s\r\n" HKPASSWORD);

    accessory->prepareIDs();

    server->setup();
}

/**
 * @brief Call it in the update routine to accept new connections and handling the accessory.
 * 
 */
void ESPHomeKit::update() {
    server->update();
    accessory->run();
}

/**
 * @brief Save the SSID in the EEPROM
 * 
 * @param ssid 
 * @param wiFiPassword 
 */
void ESPHomeKit::saveSSID(const String& ssid, const String& wiFiPassword) {
    storage->setSSID(ssid);
    storage->setPassword(wiFiPassword);
}

/**
 * @brief Get saved SSID from EEPROM
 * 
 * @return String Saved SSID in EEPROM
 */
String ESPHomeKit::getSSID() {
    return storage->getSSID();
}

/**
 * @brief Get saved WiFi Password from EEPROM
 * 
 * @return String Saved WiFi Password in EEPROM
 */
String ESPHomeKit::getWiFiPassword() {
    return storage->getPassword();
}

/**
 * @brief Get saved name, name from info service or generate name
 * 
 * @return String name
 */
String ESPHomeKit::getName() {
    if (!accessory) {
        HKLOGERROR("No Accessory in HomeKit");
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
    String accessoryId = storage->getAccessoryId();
    name += "-" + accessoryId.substring(0, 2) + accessoryId.substring(3, 5);
    return name;
}

/**
 * @brief Sets root accessory, ID has to be 1
 * 
 * @param accessory 
 */
void ESPHomeKit::setAccessory(HKAccessory *accessory) {
    if (accessory->getId() == 1) {
        ESPHomeKit::accessory = accessory;
    }
}

/**
 * @brief get accessory ID from EEPROM
 * 
 * @return String accessory ID
 */
String ESPHomeKit::getAccessoryId() {
    return storage->getAccessoryId();
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
 * @brief Get config number
 * 
 * @return int config number
 */
int ESPHomeKit::getConfigNumber() {
    return configNumber;
}

/**
 * @brief Reset EEPROM and restart ESP
 * 
 */
void ESPHomeKit::reset() {
    storage->reset();
}

/**
 * @brief Get ESPHomeKit storage manager
 * 
 * @return HKStorage* storage manager
 */
HKStorage *ESPHomeKit::getStorage() {
    return storage;
}
