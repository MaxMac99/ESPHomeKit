//
// Created by Max Vissing on 2019-06-07.
//

#ifndef LED_HAP_ESP8266_HKCLIENT_H
#define LED_HAP_ESP8266_HKCLIENT_H

#include <Arduino.h>
#include <WiFiClient.h>
#include "crypto/srp.h"
#include <ChaChaPoly.h>
#include "JSON/JSON.h"
#include <ESP8266WebServer.h>
#include "HKTLV.h"
#include "HKDefinitions.h"
#include "HKCharacteristic.h"
#include "HKStorage.h"

struct VerifyContext {
    byte accessorySecretKey[32];
    byte sharedKey[32];
    byte accessoryPublicKey[32];
    byte devicePublicKey[32];
    byte sessionKey[32];
};

class HKEvent;
class HKCharacteristic;

struct ClientEvent;

const char PROGMEM http_header_tlv8[] = "HTTP/1.1 200 OK\r\n"
                         "Content-Type: application/pairing+tlv8\r\n"
                         "Content-Length: %d\r\n"
                         "Connection: keep-alive\r\n\r\n";

const char PROGMEM http_header_json[] = "HTTP/1.1 %d %s\r\n"
                         "Content-Type: application/hap+json\r\n"
                         "Content-Length: %d\r\n"
                         "Connection: keep-alive\r\n\r\n"
                         "%s";
const char PROGMEM json_status[] = "{\"status\": %d}";
const char PROGMEM events_header_chunked[] = "EVENT/1.0 200 OK\r\n"
                                 "Content-Type: application/hap+json\r\n"
                                 "Transfer-Encoding: chunked\r\n\r\n";

class HKClient {
public:
    HKClient(WiFiClient client);
    ~HKClient();
    size_t available();
    void receive(uint8_t *message, const size_t &dataSize);
    bool readBytesWithTimeout(uint8_t *data, const size_t &maxLength, const uint32_t &timeout_ms);

    bool isConnected();
    bool isEncrypted();
    void setPairing(bool pairing);
    bool isPairing();
    uint8_t getPermission();
    int getPairingId();
    void stop();

    size_t prepareEncryption(uint8_t *accessoryPublicKey, uint8_t *encryptedResponseData, const uint8_t *devicePublicKey);
    bool finishEncryption(uint8_t *encryptedData, const size_t &encryptedSize);
    bool didStartEncryption();
    void resetEncryption();

    void send(uint8_t *message, const size_t &messageSize);
    void sendChunk(uint8_t *message, size_t messageSize);
    void sendJSONResponse(int errorCode, const char *message, const size_t &messageSize);
    void sendJSONErrorResponse(int errorCode, HAPStatus status);
    void sendTLVResponse(const std::vector<HKTLV *> &message);
    void sendTLVError(const uint8_t &state, const TLVError &error);

    void processNotifications();
    void scheduleEvent(HKCharacteristic *characteristic, HKValue newValue);

    void sendEvents(ClientEvent *event);
private:
    bool decrypt(uint8_t *decryptedMessage, size_t &decryptedSize, const uint8_t *encryptedMessage, const size_t &encryptedSize);
    void sendEncrypted(byte *message, const size_t &messageSize);
private:
    WiFiClient client;
    VerifyContext *verifyContext;
    bool encrypted;
    bool pairing;
    uint8_t readKey[32];
    int countReads;
    uint8_t writeKey[32];
    int countWrites;
    int pairingId;
    uint8_t permission;
    std::vector<HKEvent *> events;
    uint64_t lastUpdate;
};


#endif //LED_HAP_ESP8266_HKCLIENT_H
