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
#include "HKServer.h"
#include "HKCharacteristic.h"

const char json_200_response_headers[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/hap+json\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: keep-alive\r\n\r\n";

const char json_207_response_headers[] =
        "HTTP/1.1 207 Multi-Status\r\n"
        "Content-Type: application/hap+json\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: keep-alive\r\n\r\n";

struct VerifyContext {
    byte accessorySecretKey[32];
    byte sharedKey[32];
    byte accessoryPublicKey[32];
    byte devicePublicKey[32];
    byte sessionKey[32];
};

class HKServer;
class HKEvent;
struct ClientEvent;

class HKClient : public WiFiClient {
protected:
    HKClient(HKServer *server, ClientContext* client);

public:
    ~HKClient() override;
    bool received();
    void scheduleEvent(HKCharacteristic *characteristic, HKValue newValue);
    void sendEvents(ClientEvent *event);

    friend class HKServer;
private:
    bool readBytesWithTimeout(size_t maxLength, std::vector<byte> &data, int timeout_ms);
    byte *receivedDecrypted(size_t &decryptedSize);
    void send(byte *message, size_t messageSize);
    void sendChunk(byte *message, size_t messageSize);
    void sendEncrypted(byte *message, size_t messageSize);

    void sendTLVResponse(std::vector<HKTLV *> &message);
    void sendTLVError(byte state, TLVError error);
    void send204Response();
    void sendJSONResponse(int errorCode, const String& message);
    void sendJSONErrorResponse(int errorCode, HAPStatus status);

    static void hkdf(byte *target, byte *ikm, uint8_t ikmLength, byte *salt, uint8_t saltLength, byte *info, uint8_t infoLength);

    void onPairSetup(const std::vector<byte> &body);
    void onPairVerify(const std::vector<byte> &body);
    void onIdentify();
    void onGetAccessories();
    void onGetCharacteristics(String id, bool meta, bool perms, bool type, bool ev);
    void onUpdateCharacteristics(const String &jsonBody);
    HAPStatus processUpdateCharacteristic(unsigned int aid, unsigned int iid, String ev, String value);
    void onPairings(const std::vector<byte> &body);
    void onReset();
    void onResource();
private:
    HKServer *server;
    VerifyContext *verifyContext;
    bool encrypted;
    bool pairing;
    byte readKey[32];
    int countReads;
    byte writeKey[32];
    int countWrites;
    int pairingId;
    byte permission;
    std::vector<HKEvent *> events;
    uint64_t lastUpdate;
private:
    enum HTTPMethod {
        HTTP_ANY,
        HTTP_GET,
        HTTP_POST,
        HTTP_PUT,
        HTTP_PATCH,
        HTTP_DELETE,
        HTTP_OPTIONS
    };
};


#endif //LED_HAP_ESP8266_HKCLIENT_H
