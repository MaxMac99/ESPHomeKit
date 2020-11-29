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
ESPHomeKit::ESPHomeKit() : server(new HKServer(this)), accessory(nullptr), configNumber(1) {
    HKStorage::checkStorage();
}

/**
 * @brief Destroy the ESPHomeKit::ESPHomeKit object
 * 
 */
ESPHomeKit::~ESPHomeKit() {
    delete server;
}

/**
 * @brief Call it in the setup routine after you have set your accessory. Sets up the accessory and the server.
 * 
 */
void ESPHomeKit::setup() {
    HKLOGINFO("[ESPHomeKit::setup] AccessoryID: %s\r\n", HKStorage::getAccessoryId().c_str());

#if HKLOGLEVEL <= 1
    for (auto pairing : HKStorage::getPairings()) {
        HKLOGINFO("[HKStorage] Pairing id=%i deviceId=%36s permissions=%i\r\n", pairing->id, pairing->deviceId, pairing->permissions);
    }
#endif

    if (!HKStorage::isPaired()) {
        srp = new Srp(String(HKPASSWORD).c_str());
    }

    HKLOGINFO("[ESPHomeKit::setup] Password: %s\r\n", HKPASSWORD);

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
    String accessoryId = HKStorage::getAccessoryId();
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
    return HKStorage::getAccessoryId();
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
