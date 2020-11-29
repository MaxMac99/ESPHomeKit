/**
 * @file ESPHomeKit.h
 * @author Max Vissing (max_vissing@yahoo.de)
 * @brief root module for ESPHomeKit
 * @version 0.1
 * @date 2020-05-25
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef HAP_SERVER_HOMEKIT_H
#define HAP_SERVER_HOMEKIT_H

#include <Arduino.h>

#ifndef HKPASSWORD
#define HKPASSWORD "123-45-678"
#pragma message ("WARNING: Using default Password 123-45-678")
#endif

#ifndef HKSETUPID
#pragma message ("WARNING: No setup id given")
#endif

#include "HKDebug.h"
#include "HKStorage.h"
#include "HKAccessory.h"
#include "HKServer.h"

class HKAccessory;
class HKServer;

class ESPHomeKit {
public:
    explicit ESPHomeKit();
    ~ESPHomeKit();
    void setup();
    void update();
    void reset();
    void resetPairings();

    void saveSSID(const String& ssid, const String& wiFiPassword="");
    String getSSID();
    String getWiFiPassword();
    String getName();

    void setAccessory(HKAccessory *accessory);
    HKAccessory *getAccessory();

    String getAccessoryId();
    int getConfigNumber();

    HKStorage *getStorage();

    friend class HKClient;
    friend class HKServer;
private:
    HKStorage *storage;
    HKServer *server;
    HKAccessory *accessory;
    Srp *srp;

    int configNumber;
};


#endif //HAP_SERVER_HOMEKIT_H
