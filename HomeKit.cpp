/**
 * @file HomeKit.cpp
 * @author Max Vissing (max_vissing@yahoo.de)
 * @brief 
 * @version 0.1
 * @date 2020-05-25
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "HomeKit.h"
#include "crypto/srp.h"

/**
 * @brief Construct a new HomeKit::HomeKit object
 * 
 * @param password Password of HomeKit when registering; has to be in the format "123-45-678"
 * @param setupId Used when generating a QR-Code; Has to be 4 characters long
 * @param name Name of HomeKit object; automatically generated when not specified
 */
HomeKit::HomeKit(String password, String setupId, String name) : password(std::move(password)), storage(new HKStorage()), server(new HKServer(this)), accessory(nullptr), setupId(std::move(setupId)), name(std::move(name)), configNumber(1) {
}

/**
 * @brief Destroy the HomeKit::HomeKit object
 * 
 */
HomeKit::~HomeKit() {
    delete server;
    delete storage;
}

/**
 * @brief Call it in the setup routine after you have set your accessory. Sets up the accessory and the server.
 * 
 */
void HomeKit::setup() {
    HKLOGINFO("[HomeKit::setup] AccessoryID: %s\r\n", storage->getAccessoryId().c_str());

#if HKLOGLEVEL <= 1
    for (auto pairing : storage->getPairings()) {
        HKLOGINFO("[HKStorage] Pairing id=%i deviceId=%36s permissions=%i\r\n", pairing->id, pairing->deviceId, pairing->permissions);
    }
#endif

    srp_init(password.c_str());

    HKLOGINFO("[HomeKit::setup] Password: %s\r\n", (char *) srp_pinMessage() + 11);

    accessory->prepareIDs();

    server->setup();
}

/**
 * @brief Call it in the update routine to accept new connections and handling the accessory.
 * 
 */
void HomeKit::update() {
    server->update();
    accessory->run();
}

/**
 * @brief Save the SSID in the EEPROM
 * 
 * @param ssid 
 * @param wiFiPassword 
 */
void HomeKit::saveSSID(const String& ssid, const String& wiFiPassword) {
    storage->setSSID(ssid);
    storage->setPassword(wiFiPassword);
}

/**
 * @brief Get saved SSID from EEPROM
 * 
 * @return String Saved SSID in EEPROM
 */
String HomeKit::getSSID() {
    return storage->getSSID();
}

/**
 * @brief Get saved WiFi Password from EEPROM
 * 
 * @return String Saved WiFi Password in EEPROM
 */
String HomeKit::getWiFiPassword() {
    return storage->getPassword();
}

/**
 * @brief Get saved name, name from info service or generate name
 * 
 * @return String name
 */
String HomeKit::getName() {
    if (name == "") {
        if (accessory) {
            HKService *info = accessory->getService(HKServiceAccessoryInfo);
            if (!info) {
                return generateCustomName();
            }

            HKCharacteristic *serviceName = info->getCharacteristic(HKCharacteristicName);
            if (!serviceName) {
                return generateCustomName();
            }

            name = serviceName->getValue().stringValue;
            String accessoryId = storage->getAccessoryId();
            name += "-" + accessoryId.substring(0, 2) + accessoryId.substring(3, 5);
            return name;
        }
        return generateCustomName();
    }
    return name;
}

/**
 * @brief Generates custom name in format "MyHomeKit-<AccessoryId>"
 * 
 * @return String custom generated name
 */
String HomeKit::generateCustomName() {
    name = "MyHomeKit-" + storage->getAccessoryId();
    return name;
}

/**
 * @brief Sets root accessory, ID has to be 1
 * 
 * @param accessory 
 */
void HomeKit::setAccessory(HKAccessory *accessory) {
    if (accessory->getId() == 1) {
        HomeKit::accessory = accessory;
    }
}

/**
 * @brief get accessory ID from EEPROM
 * 
 * @return String accessory ID
 */
String HomeKit::getAccessoryId() {
    return storage->getAccessoryId();
}

/**
 * @brief Get accessory
 * 
 * @return HKAccessory* accessory
 */
HKAccessory *HomeKit::getAccessory() {
    return accessory;
}

/**
 * @brief Get config number
 * 
 * @return int config number
 */
int HomeKit::getConfigNumber() {
    return configNumber;
}

/**
 * @brief Reset EEPROM and restart ESP
 * 
 */
void HomeKit::reset() {
    storage->reset();
}

/**
 * @brief Get HomeKit Password
 * 
 * @return String password
 */
String HomeKit::getPassword() {
    return password;
}

/**
 * @brief Get HomeKit storage manager
 * 
 * @return HKStorage* storage manager
 */
HKStorage *HomeKit::getStorage() {
    return storage;
}
