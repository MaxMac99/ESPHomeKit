//
// Created by Max Vissing on 2019-06-01.
//

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
