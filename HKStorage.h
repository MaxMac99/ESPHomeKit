//
// Created by Max Vissing on 2019-04-27.
//

#ifndef HAP_SERVER_HKSTORAGE_H
#define HAP_SERVER_HKSTORAGE_H

#include <Arduino.h>
#include <EEPROM.h>
#include <Ed25519.h>

#include "HKDebug.h"
#include "HKDefinitions.h"

#ifndef STORAGE_BASE_ADDR
#define STORAGE_BASE_ADDR   0x0
#endif

#define MAX_PAIRINGS 16

#define ACCESSORY_ID_SIZE   17

#define COMPARE_SIZE 3

#define COMPARING String("MAX").c_str()

#ifdef HKSETUPID
#define STORAGE_CHECK HKSETUPID
#else
#define STORAGE_CHECK "MAXH"
#endif
#define STORAGE_CHECK_LEN 4

struct Pairing {
    int id;
    char deviceId[36];
    byte deviceKey[32];
    unsigned char permissions;
};

struct KeyPair {
    byte privateKey[32];
    byte publicKey[32];
};

#define STORAGE_CHECK_ADDR  STORAGE_BASE_ADDR
#define SSID_ADDR           (STORAGE_CHECK_ADDR + STORAGE_CHECK_LEN)
#define WIFI_PASSWORD_ADDR  (SSID_ADDR + 32)
#define ACCESSORY_ID_ADDR   (WIFI_PASSWORD_ADDR + 64)
#define ACCESSORY_KEY_ADDR  (ACCESSORY_ID_ADDR + 6)
#define PAIRINGS_ADDR       (ACCESSORY_KEY_ADDR + sizeof(KeyPair))

namespace HKStorage {
    void checkStorage();
    void reset();
    void resetPairings();
    void saveSSID(const String &ssid);
    String getSSID();
    void saveWiFiPassword(const String &password);
    String getWiFiPassword();
    
    String getAccessoryId();
    KeyPair getAccessoryKey();

    bool isPaired();
    bool hasPairedAdmin();
    std::vector<Pairing *> getPairings();
    int addPairing(const char *deviceId, const byte *deviceKey, byte permission);
    Pairing *findPairing(const char *deviceId);
    int updatePairing(const String &deviceId, byte permission);
    int removePairing(const String &deviceId);
    
    struct PairingData {
        char comparing[COMPARE_SIZE];
        unsigned char permissions;
        char deviceId[36];
        byte devicePublicKey[32];
    };
};


#endif //HAP_SERVER_HKSTORAGE_H
