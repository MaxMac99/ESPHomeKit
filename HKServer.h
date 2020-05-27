//
// Created by Max Vissing on 2019-04-26.
//

#ifndef HAP_SERVER_HKSERVER_H
#define HAP_SERVER_HKSERVER_H

#define PORT 5556

#define MAX_CLIENTS 16

#include <Arduino.h>
#include <ESP8266mDNS.h>
#include <WiFiServer.h>
#include "HomeKit.h"
#include "HKClient.h"

class HomeKit;
class HKClient;

class HKServer : private WiFiServer {
public:
    explicit HKServer(HomeKit *hk);
    void setup();
    void update();
    bool isPairing();

    friend class HKClient;
private:
    int setupMDNS();
    HKClient *availableHK(uint8_t* status = NULL);
    void processNotificationsFromClient();
private:
    HomeKit *hk;

    esp8266::MDNSImplementation::MDNSResponder::hMDNSService mdnsService;

    std::vector<HKClient *> clients;
};


#endif //HAP_SERVER_HKSERVER_H
