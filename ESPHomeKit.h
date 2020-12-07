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
#include <ESP8266mDNS.h>
#include <WiFiServer.h>
#include <base64.h>

#ifndef HKPASSWORD
#define HKPASSWORD "123-45-678"
#pragma message ("WARNING: Using default Password 123-45-678")
#endif

#ifndef HKSETUPID
#pragma message ("WARNING: No setup id given")
#endif

#define PORT 5556
#define MAX_CLIENTS 16
#define NOTIFICATION_UPDATE_FREQUENCY 1000

#include "HKDebug.h"
#include "HKStorage.h"
#include "HKAccessory.h"
#include "HKClient.h"

class HKAccessory;
class HKClient;

const char PROGMEM http_header_200_chunked[] = "HTTP/1.1 200 OK\r\n"
                                        "Content-Type: application/hap+json\r\n"
                                        "Transfer-Encoding: chunked\r\n"
                                        "Connection: keep-alive\r\n\r\n";
const char PROGMEM http_header_207_chunked[] = "HTTP/1.1 207 Multi-Status\r\n"
                                        "Content-Type: application/hap+json\r\n"
                                        "Transfer-Encoding: chunked\r\n"
                                        "Connection: keep-alive\r\n\r\n";
const char PROGMEM http_header_204[] = "HTTP/1.1 204 No Content\r\n\r\n";

class ESPHomeKit {
public:
    explicit ESPHomeKit();
    ~ESPHomeKit();
    void setup(HKAccessory *accessory);
    void begin();
    void update();
    bool isPairing();
    void reset();
    void resetPairings();

    String getName();
    HKAccessory *getAccessory();

    friend class HKClient;
private:
    bool setupMDNS();
    void handleClient();
    void parseMessage(HKClient *client, uint8_t *message, const size_t &messageSize);
    
    void onGetAccessories(HKClient *client);
    void onGetCharacteristics(HKClient *client, String id, bool meta, bool perms, bool type, bool ev);
    void onIdentify(HKClient *client);
    void onUpdateCharacteristics(HKClient *client, uint8_t *message, const size_t &messageSize);
    HAPStatus processUpdateCharacteristic(HKClient *client, uint aid, uint iid, String ev, String value);

    void onPairSetup(HKClient *client, uint8_t *message, const size_t &messageSize);
    void onPairVerify(HKClient *client, uint8_t *message, const size_t &messageSize);
    void onPairings(HKClient *client, uint8_t *message, const size_t &messageSize);
private:
    // Server
    WiFiServer server;
    esp8266::MDNSImplementation::MDNSResponder::hMDNSService mdnsService;
    std::vector<HKClient *> clients;

    HKAccessory *accessory;
    Srp *srp;

    int configNumber;
};


#endif //HAP_SERVER_HOMEKIT_H
