//
// Created by Max Vissing on 2019-04-27.
//

#include "HKStorage.h"

// Helper Functions
int findEmptyBlock() {
    unsigned char pairingData[3];
    EEPROM.begin(4096);
    for (int i = 0; i < MAX_PAIRINGS; i++) {
        EEPROM.get(PAIRINGS_ADDR + sizeof(HKStorage::PairingData)*i, pairingData);

        bool empty = true;
        for (unsigned char j : pairingData) {
            if (j != 0x0) {
                empty = false;
                break;
            }
        }

        if (empty) {
            EEPROM.end();
            return i;
        }
    }

    EEPROM.end();
    return -1;
}

String generateAccessoryId() {
    EEPROM.begin(4096);
    String result = String();
    for (uint8_t i = 0; i < 6; i++) {
        uint8_t number = (uint8_t) random(0xFF);
        EEPROM.write(ACCESSORY_ID_ADDR + i, number);
        if (number < 0x10) {
            result += "0" + String(number, HEX) + ":";
        } else {
            result += String(number, HEX) + ":";
        }
    }
    result.toUpperCase();
    EEPROM.commit();
    EEPROM.end();

    HKLOGINFO("[HKStorage::generateAccessoryId] Accessory ID: %s\r\n", result.c_str());

    return result;
}

KeyPair generateAccessoryKey() {
    EEPROM.begin(4096);
    HKLOGINFO("[HKStorage::generateAccessoryKey] Generating Accessory Key\r\n");
    KeyPair result{};
    os_get_random(result.privateKey, sizeof(result.privateKey));
    Ed25519::derivePublicKey(result.publicKey, result.privateKey);
    
    EEPROM.put(ACCESSORY_KEY_ADDR, result);
    EEPROM.commit();
    EEPROM.end();

    return result;
}

void writeString(uint16_t address, String data, uint16_t maxLength) {
    EEPROM.begin(4096);
    for (uint16_t i = 0; i < data.length() && (maxLength == 0 || i < maxLength); i++) {
        EEPROM.write(address + i, data[i]);
    }
    if (maxLength == 0 || data.length() < maxLength) {
        EEPROM.write(address + data.length(), '\0');
    }
    EEPROM.commit();
    EEPROM.end();
}

String readString(uint16_t address, uint16_t maxLength) {
    String data = String();
    char k;
    EEPROM.begin(4096);
    k = EEPROM.read(address);
    while (k != '\0' && (maxLength == 0 || data.length() < maxLength)) {
        data += k;
        k = EEPROM.read(address + data.length());
    }
    EEPROM.end();
    return data;
}

/**
 * @brief Check if storage is still valid
 * 
 */
void HKStorage::checkStorage() {
    String comparing = readString(STORAGE_CHECK_ADDR, STORAGE_CHECK_LEN);
    if (comparing != STORAGE_CHECK) {
        reset();
    }
}

/**
 * @brief Reset complete EEPROM
 * 
 */
void HKStorage::reset() {
    HKLOGINFO("[HKStorage::reset] Reset\r\n");
    writeString(STORAGE_CHECK_ADDR, STORAGE_CHECK, STORAGE_CHECK_LEN);
    EEPROM.begin(4096);
    for (uint i = STORAGE_CHECK_ADDR + STORAGE_CHECK_LEN; i < 4096; i++) {
        EEPROM.write(i, 0);
    }
    EEPROM.end();
}

/**
 * @brief Remove all paired clients
 * 
 */
void HKStorage::resetPairings() {
    EEPROM.begin(4096);
    for (uint i = PAIRINGS_ADDR; i < 4096; i++) {
        EEPROM.write(i, 0);
    }
    EEPROM.end();
    HKLOGINFO("[HKStorage::resetPairings] Reset Pairings\r\n");
}

/**
 * @brief Get stored accessory id
 * 
 * @return String Accessory Id
 */
String HKStorage::getAccessoryId() {
    EEPROM.begin(4096);
    String result = String();
    bool generate = true;
    for (uint8_t i = 0; i < 6; i++) {
        uint8_t number = EEPROM.read(ACCESSORY_ID_ADDR + i);
        if (number != 0) {
            generate = false;
        }
        if (number < 0x10) {
            result += "0" + String(number, HEX) + ":";
        } else {
            result += String(number, HEX) + ":";
        }
    }
    EEPROM.end();
    if (generate) {
        result = generateAccessoryId();
    }
    result.toUpperCase();
    return result;
}

/**
 * @brief Key pair for accessory
 * 
 * @return KeyPair Key pair (public/private)
 */
KeyPair HKStorage::getAccessoryKey() {
    KeyPair zero{};
    KeyPair result{};
    EEPROM.begin(4096);
    EEPROM.get(ACCESSORY_KEY_ADDR, result);
    EEPROM.end();

    if (memcmp(result.privateKey, zero.privateKey, sizeof(result.privateKey)) == 0 || memcmp(result.publicKey, zero.publicKey, sizeof(result.publicKey)) == 0) {
        result = generateAccessoryKey();
    }

    return result;
}

/**
 * @brief Store SSID in EEPROM
 * 
 * @param ssid SSID to save
 */
void HKStorage::saveSSID(const String &ssid) {
    writeString(SSID_ADDR, ssid, 32);
    HKLOGINFO("[HKStorage::resetPairings] Set ssid %s\r\n", readString(SSID_ADDR, 32).c_str());
}

/**
 * @brief Store WiFi Password in EEPROM
 * 
 * @param password Password to save
 */
void HKStorage::saveWiFiPassword(const String &password) {
    writeString(WIFI_PASSWORD_ADDR, password, 64);
    HKLOGINFO("[HKStorage::resetPairings] Set password %s\r\n", readString(WIFI_PASSWORD_ADDR, 64).c_str());
}

/**
 * @brief Get saved SSID
 * 
 * @return String SSID
 */
String HKStorage::getSSID() {
    return readString(SSID_ADDR, 32);
}

/**
 * @brief Get saved WiFi Password
 * 
 * @return String Password
 */
String HKStorage::getWiFiPassword() {
    return readString(WIFI_PASSWORD_ADDR, 64);
}

/**
 * @brief Is there a paired client
 * 
 * @return true Contains paired client
 * @return false No paired clients
 */
bool HKStorage::isPaired() {
    PairingData pairingData{};
    EEPROM.begin(4096);
    for (int i = 0; i < MAX_PAIRINGS; i++) {
        EEPROM.get(PAIRINGS_ADDR +  + sizeof(pairingData)*i, pairingData);
        if (strncmp(pairingData.comparing, COMPARING, COMPARE_SIZE) != 0) {
            continue;
        }
        EEPROM.end();
        return true;
    }
    EEPROM.end();
    return false;
}

/**
 * @brief Save paired client
 * 
 * @param deviceId Device Id
 * @param deviceKey Public key of device
 * @param permission Permissions for device
 * @return int Successfully saved = 0
 */
int HKStorage::addPairing(const char *deviceId, const byte *deviceKey, byte permission) {
    int nextBlockIdx = findEmptyBlock();

    if (nextBlockIdx == -1) {
        //Failed to write pairing info to flash: max number of pairings
        return -2;
    }

    PairingData pairingData{};
    strncpy(pairingData.comparing, COMPARING, COMPARE_SIZE);
    pairingData.permissions = permission;
    strncpy(pairingData.deviceId, deviceId, sizeof(pairingData.deviceId));
    memcpy(pairingData.devicePublicKey, deviceKey, sizeof(pairingData.devicePublicKey));

    EEPROM.begin(4096);
    EEPROM.put(PAIRINGS_ADDR + sizeof(pairingData)*nextBlockIdx, pairingData);
    EEPROM.end();

    return 0;
}

/**
 * @brief Update permissions for stored device
 * 
 * @param deviceId Device Id
 * @param permission Updated permissions for device
 * @return int Successfully changed (0)
 */
int HKStorage::updatePairing(const String &deviceId, byte permission) {
    PairingData pairingData{};
    EEPROM.begin(4096);
    for (int i = 0; i < MAX_PAIRINGS; i++) {
        EEPROM.get(PAIRINGS_ADDR + sizeof(pairingData)*i, pairingData);
        if (strncmp(pairingData.comparing, COMPARING, COMPARE_SIZE) != 0) {
            continue;
        }

        if (strncmp(pairingData.deviceId, deviceId.c_str(), sizeof(pairingData.deviceId)) == 0) {
            pairingData.permissions = permission;
            EEPROM.put(PAIRINGS_ADDR + sizeof(pairingData)*i, pairingData);
            EEPROM.end();
            return 0;
        }
    }
    EEPROM.end();
    return -1;
}

/**
 * @brief Find paired device with Id
 * 
 * @param deviceId Devie to find
 * @return Pairing* Stored data for device
 */
Pairing *HKStorage::findPairing(const char *deviceId) {
    PairingData pairingData{};
    EEPROM.begin(4096);
    for (int i = 0; i < MAX_PAIRINGS; i++) {
        EEPROM.get(PAIRINGS_ADDR + sizeof(pairingData)*i, pairingData);
        if (strncmp(pairingData.comparing, COMPARING, COMPARE_SIZE) != 0) {
            continue;
        }

        if (memcmp(pairingData.deviceId, deviceId, sizeof(pairingData.deviceId)) == 0) {
            auto pairing = new Pairing();
            pairing->id = i;
            strncpy(pairing->deviceId, pairingData.deviceId, sizeof(pairingData.deviceId));
            memcpy(pairing->deviceKey, pairingData.devicePublicKey, sizeof(pairingData.devicePublicKey));
            pairing->permissions = pairingData.permissions;

            EEPROM.end();
            return pairing;
        }
    }
    EEPROM.end();
    return nullptr;
}

/**
 * @brief Remove paired device
 * 
 * @param deviceId Device to remove
 * @return int Successful (0)
 */
int HKStorage::removePairing(const String &deviceId) {
    PairingData pairingData{};
    EEPROM.begin(4096);
    for (int i = 0; i < MAX_PAIRINGS; i++) {
        EEPROM.get(PAIRINGS_ADDR + sizeof(pairingData)*i, pairingData);
        if (strncmp(pairingData.comparing, COMPARING, COMPARE_SIZE) != 0) {
            continue;
        }

        if (strncmp(pairingData.deviceId, deviceId.c_str(), sizeof(pairingData.deviceId)) == 0) {
            PairingData empty{};
            EEPROM.put(PAIRINGS_ADDR + sizeof(pairingData)*i, empty);
            EEPROM.end();
            return 0;
        }
    }
    EEPROM.end();
    return -1;
}

/**
 * @brief Is device with Admin rights paired
 * 
 * @return true Device with Admin rights
 * @return false No device with Admin rights
 */
bool HKStorage::hasPairedAdmin() {
    PairingData pairingData{};
    EEPROM.begin(4096);
    for (int i = 0; i < MAX_PAIRINGS; i++) {
        EEPROM.get(PAIRINGS_ADDR + sizeof(pairingData)*i, pairingData);
        if (strncmp(pairingData.comparing, COMPARING, COMPARE_SIZE) != 0) {
            continue;
        }

        if (pairingData.permissions & PairingPermissionAdmin) {
            EEPROM.end();
            return true;
        }
    }
    EEPROM.end();
    return false;
}

/**
 * @brief Get all connected devices
 * 
 * @return std::vector<Pairing *> List of connected devices
 */
std::vector<Pairing *> HKStorage::getPairings() {
    std::vector<Pairing *> pairings;
    PairingData pairingData{};
    EEPROM.begin(4096);
    for (int i = 0; i < MAX_PAIRINGS; i++) {
        EEPROM.get(PAIRINGS_ADDR + sizeof(pairingData)*i, pairingData);
        if (strncmp(pairingData.comparing, COMPARING, COMPARE_SIZE) != 0) {
            continue;
        }

        auto pairing = new Pairing();
        pairing->id = i;
        strncpy(pairing->deviceId, pairingData.deviceId, sizeof(pairingData.deviceId));
        memcpy(pairing->deviceKey, pairingData.devicePublicKey, sizeof(pairingData.devicePublicKey));
        pairing->permissions = pairingData.permissions;
        pairings.push_back(pairing);
    }
    EEPROM.end();
    return pairings;
}
