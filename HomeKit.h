/**
 * @file HomeKit.h
 * @author Max Vissing (max_vissing@yahoo.de)
 * @brief root module for HomeKit
 * @version 0.1
 * @date 2020-05-25
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef HAP_SERVER_HOMEKIT_H
#define HAP_SERVER_HOMEKIT_H

#include <Arduino.h>

#include "HKDebug.h"
#include "HKStorage.h"
#include "HKAccessory.h"
#include "HKServer.h"

class HKAccessory;
class HKServer;

class HomeKit {
public:
    explicit HomeKit(String password, String setupId="", String name="");
    ~HomeKit();
    void setup();
    void update();
    void reset();

    void saveSSID(const String& ssid, const String& wiFiPassword="");
    String getSSID();
    String getWiFiPassword();
    String getName();

    void setAccessory(HKAccessory *accessory);
    HKAccessory *getAccessory();

    String getAccessoryId();
    int getConfigNumber();

    HKStorage *getStorage();
    String getPassword();

    friend class HKClient;
    friend class HKServer;
private:
    String generateCustomName();
private:
    String password;
    HKStorage *storage;
    HKServer *server;
    HKAccessory *accessory;

    String setupId;
    String name;
    int configNumber;
};


#endif //HAP_SERVER_HOMEKIT_H
