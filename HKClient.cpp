//
// Created by Max Vissing on 2019-06-07.
//

#include "HKClient.h"

HKClient::HKClient(WiFiClient client) : client(client), verifyContext(nullptr), encrypted(false), pairing(false), readKey(), countReads(0), writeKey(), countWrites(0), pairingId(0), permission(0) {}

HKClient::~HKClient() {
    delete verifyContext;
}

size_t HKClient::available() {
    return client.available();
}

void HKClient::receive(uint8_t *message, const size_t &dataSize) {
    if (!dataSize) {
        return;
    }
    client.readBytes(message, dataSize);

    if (encrypted) {
        size_t decryptedSize = 1;
        decrypt(nullptr, decryptedSize, nullptr, dataSize);
        uint8_t decryptedMessage[decryptedSize];
        if (!decrypt(decryptedMessage, decryptedSize, message, dataSize)) {
            return;
        }
    }
}

bool HKClient::readBytesWithTimeout(uint8_t *data, const size_t &maxLength, const uint32_t &timeout_ms) {
    size_t currentLength = 0;
    while (currentLength < maxLength) {
        uint32_t tries = timeout_ms;
        size_t avail;
        while (!(avail = client.available()) && tries-- != 0) {
            delay(1);
        }
        if (!avail) {
            break;
        }
        if (currentLength + avail > maxLength) {
            avail = maxLength - currentLength;
        }
        client.readBytes(data + currentLength, avail);
        currentLength += avail;
    }
    return currentLength == maxLength;
}

bool HKClient::isConnected() {
    return client.connected();
}

bool HKClient::isEncrypted() {
    return encrypted;
}

void HKClient::setPairing(bool pairing) {
    this->pairing = pairing;
}

bool HKClient::isPairing() {
    return pairing;
}

uint8_t HKClient::getPermission() {
    return permission;
}

int HKClient::getPairingId() {
    return pairingId;
}

void HKClient::stop() {
    client.stop();
}

size_t HKClient::prepareEncryption(uint8_t *accessoryPublicKey, uint8_t *encryptedResponseData, const uint8_t *devicePublicKey) {
    if (encryptedResponseData == nullptr) {
        std::vector<HKTLV *> dummySubResponseMessage = {
            new HKTLV(TLVTypeIdentifier, nullptr, HKStorage::getAccessoryId().length()),
            new HKTLV(TLVTypeSignature, nullptr, 64)
        };
        return HKTLV::getFormattedTLVSize(dummySubResponseMessage) + 16;
    }
    if (verifyContext == nullptr) {
        verifyContext = new VerifyContext();
    }
    memcpy(verifyContext->devicePublicKey, devicePublicKey, 32);

    os_get_random(verifyContext->accessorySecretKey, 32);

    crypto_scalarmult_curve25519_base(verifyContext->accessoryPublicKey, verifyContext->accessorySecretKey);
    crypto_scalarmult_curve25519(verifyContext->sharedKey, verifyContext->accessorySecretKey, verifyContext->devicePublicKey);

    memcpy(accessoryPublicKey, verifyContext->accessoryPublicKey, 32);

    String accessoryId = HKStorage::getAccessoryId();
    size_t accessoryInfoSize = 32 + accessoryId.length() + 32;
    uint8_t accessoryInfo[accessoryInfoSize];
    memcpy(accessoryInfo, accessoryPublicKey, 32);
    memcpy(accessoryInfo + 32, accessoryId.c_str(), accessoryId.length());
    memcpy(accessoryInfo + 32 + accessoryId.length(), devicePublicKey, 32);

    uint8_t accessorySignature[64];
    Ed25519::sign(accessorySignature, HKStorage::getAccessoryKey().privateKey, HKStorage::getAccessoryKey().publicKey, accessoryInfo, accessoryInfoSize);

    std::vector<HKTLV *> subResponseMessage = {
            new HKTLV(TLVTypeIdentifier, (uint8_t *) accessoryId.c_str(), accessoryId.length()),
            new HKTLV(TLVTypeSignature, accessorySignature, 64)
    };
    size_t responseSize = HKTLV::getFormattedTLVSize(subResponseMessage);
    uint8_t responseData[responseSize];
    HKTLV::formatTLV(subResponseMessage, responseData);
    if (encryptedResponseData == nullptr) {
        return responseSize + 16;
    }

    const char salt1[] = "Pair-Verify-Encrypt-Salt";
    const char info1[] = "Pair-Verify-Encrypt-Info\001";
    hkdf(verifyContext->sessionKey, verifyContext->sharedKey, 32, (uint8_t *) salt1, sizeof(salt1)-1, (uint8_t *) info1, sizeof(info1)-1);

    crypto_encryptAndSeal(verifyContext->sessionKey, (uint8_t *) "PV-Msg02", responseData, responseSize, encryptedResponseData, encryptedResponseData + responseSize);

    return responseSize + 16;
}

bool HKClient::finishEncryption(uint8_t *encryptedData, const size_t &encryptedSize) {
    size_t decryptedDataSize = encryptedSize - 16;
    byte decryptedData[decryptedDataSize];
    if (!crypto_verifyAndDecrypt(verifyContext->sessionKey, (byte *) "PV-Msg03", encryptedData, decryptedDataSize, decryptedData, encryptedData + decryptedDataSize)) {
        HKLOGINFO("[HKClient::onPairVerify] Could not verify message\r\n");
        delete verifyContext;
        verifyContext = nullptr;
        return false;
    }

    std::vector<HKTLV *> decryptedMessage = HKTLV::parseTLV(decryptedData, decryptedDataSize);
    HKTLV *deviceId = HKTLV::findTLV(decryptedMessage, TLVTypeIdentifier);
    if (!deviceId) {
        HKLOGINFO("[HKClient::onPairVerify] Could not find device ID\r\n");
        for (auto msg : decryptedMessage) {
            delete msg;
        }
        delete verifyContext;
        verifyContext = nullptr;
        return false;
    }

    HKTLV *deviceSignature = HKTLV::findTLV(decryptedMessage, TLVTypeSignature);
    if (!deviceSignature) {
        HKLOGINFO("[HKClient::onPairVerify] Could not find device Signature\r\n");
        for (auto msg : decryptedMessage) {
            delete msg;
        }
        delete verifyContext;
        verifyContext = nullptr;
        return false;
    }

    Pairing *pairingItem = HKStorage::findPairing((char *) deviceId->getValue());
    if (!pairingItem) {
        HKLOGINFO("[HKClient::onPairVerify] Device is not paired\r\n");
        for (auto msg : decryptedMessage) {
            delete msg;
        }
        delete verifyContext;
        verifyContext = nullptr;
        return false;
    }

    size_t deviceInfoSize = sizeof(verifyContext->devicePublicKey) + sizeof(verifyContext->accessoryPublicKey) + deviceId->getSize();
    byte deviceInfo[deviceInfoSize];
    memcpy(deviceInfo, verifyContext->devicePublicKey, sizeof(verifyContext->devicePublicKey));
    memcpy(deviceInfo + sizeof(verifyContext->devicePublicKey), deviceId->getValue(), deviceId->getSize());
    memcpy(deviceInfo + sizeof(verifyContext->devicePublicKey) + deviceId->getSize(), verifyContext->accessoryPublicKey, sizeof(verifyContext->accessoryPublicKey));

    if (!Ed25519::verify(deviceSignature->getValue(), pairingItem->deviceKey, deviceInfo, deviceInfoSize)) {
        HKLOGINFO("[HKClient::onPairVerify] Could not verify device readInfo\r\n");
        delete pairingItem;
        for (auto msg : decryptedMessage) {
            delete msg;
        }
        delete verifyContext;
        verifyContext = nullptr;
        return false;
    }

    const byte salt[] = "Control-Salt";
    const byte readInfo[] = "Control-Read-Encryption-Key\001";
    hkdf(readKey, verifyContext->sharedKey, 32, (uint8_t *) salt, sizeof(salt)-1, (uint8_t *) readInfo, sizeof(readInfo)-1);

    const byte writeInfo[] = "Control-Write-Encryption-Key\001";
    hkdf(writeKey, verifyContext->sharedKey, 32, (uint8_t *) salt, sizeof(salt)-1, (uint8_t *) writeInfo, sizeof(writeInfo)-1);

    pairingId = pairingItem->id;
    permission = pairingItem->permissions;
    delete pairingItem;

    for (auto msg : decryptedMessage) {
        delete msg;
    }
    delete verifyContext;
    verifyContext = nullptr;
    encrypted = true;

    return true;
}

bool HKClient::didStartEncryption() {
    return verifyContext != nullptr;
}

void HKClient::resetEncryption() {
    if (verifyContext) {
        delete verifyContext;
        verifyContext = nullptr;
    }
}

void HKClient::send(uint8_t *message, const size_t &messageSize) {
    #if HKLOGLEVEL == 0
    HKLOGDEBUGSINGLE("------------- Sending -------------\r\n");
    for (size_t i = 0; i < messageSize; i++) {
        byte item = *(message + i);
        if (item == '\r' || item == '\n' || (item >= ' ' && item <= '}' && item != '\\')) {
            HKLOGDEBUGSINGLE("%c", item);
        } else if (item < 0x10) {
            HKLOGDEBUGSINGLE("\\x0%x", item);
        } else {
            HKLOGDEBUGSINGLE("\\x%x", item);
        }
    }
    HKLOGDEBUGSINGLE("\r\n----------- End Sending -----------\r\n");
    #endif

    if (encrypted) {
        sendEncrypted(message, messageSize);
    } else {
        HKLOGINFO("[HKClient::send] heap: %d length: %d\r\n", ESP.getFreeHeap(), messageSize);
        client.write(message, messageSize);
        HKLOGINFO("[HKClient::send] heap: %d\r\n", ESP.getFreeHeap());
    }
}

void HKClient::sendChunk(uint8_t *message, size_t messageSize) {
    size_t payloadSize = messageSize + 8;
    uint8_t payload[payloadSize];

    int offset = snprintf((char *) payload, payloadSize, "%x\r\n", messageSize);
    memcpy(payload + offset, message, messageSize);
    payload[offset + messageSize] = '\r';
    payload[offset + messageSize + 1] = '\n';

    send(payload, offset + messageSize + 2);
}

void HKClient::sendJSONResponse(int errorCode, const char *message, const size_t &messageSize) {
    String statusText;
    switch (errorCode) {
        case 204: statusText = F("No Content"); break;
        case 207: statusText = F("Multi-Status"); break;
        case 400: statusText = F("Bad Request"); break;
        case 404: statusText = F("Not Found"); break;
        case 422: statusText = F("Unprocessable Entity"); break;
        case 500: statusText = F("Internal Server Error"); break;
        case 503: statusText = F("Service Unavailable"); break;
        default: statusText = F("OK"); break;
    }
    size_t maxSize = sizeof(http_header_json) + messageSize + 28;
    char header[maxSize];
    size_t currentSize = snprintf_P(header, maxSize, http_header_json, errorCode, statusText.c_str(), messageSize, message);

    send((uint8_t *) header, currentSize);
}

void HKClient::sendJSONErrorResponse(int errorCode, HAPStatus status) {
    char message[sizeof(json_status)+4];
    size_t currentSize = snprintf_P(message, sizeof(json_status) + 4, json_status, status);

    sendJSONResponse(errorCode, message, currentSize);
}

void HKClient::sendTLVResponse(const std::vector<HKTLV *> &message) {
    HKLOGINFO("[HKClient::sendTLVResponse] heap: %d\r\n", ESP.getFreeHeap());
    size_t bodySize = HKTLV::getFormattedTLVSize(message);
    HKLOGINFO("[HKClient::sendTLVResponse] heap: %d\r\n", ESP.getFreeHeap());

    uint8_t body[bodySize];
    HKLOGINFO("[HKClient::sendTLVResponse] heap: %d\r\n", ESP.getFreeHeap());
    HKTLV::formatTLV(message, body);
    HKLOGINFO("[HKClient::sendTLVResponse] heap: %d\r\n", ESP.getFreeHeap());
    // HKLOGDEBUGSINGLE("TLV formatted: \r\n");
    // for (size_t i = 0; i < bodySize;) {
    //     if (i < 0x10) {
    //         HKLOGDEBUGSINGLE("000%x:   ", i);
    //     } else if (i < 0x100) {
    //         HKLOGDEBUGSINGLE("00%x:   ", i);
    //     } else if (i < 0x1000) {
    //         HKLOGDEBUGSINGLE("0%x:   ", i);
    //     } else {
    //         HKLOGDEBUGSINGLE("%x:   ", i);
    //     }
    //     for (size_t j = 0; j < 8 && i < bodySize; j++) {
    //         byte item = *(body + i++);
    //         if (item < 0x10) {
    //             HKLOGDEBUGSINGLE("0%x ", item);
    //         } else {
    //             HKLOGDEBUGSINGLE("%x ", item);
    //         }
    //     }
    //     HKLOGDEBUGSINGLE("\r\n");
    // }
    // HKLOGDEBUGSINGLE("\r\nTLV formatted end\r\n");

    size_t headersSize = sizeof(http_header_tlv8) + 3; // content-lenght max 5 characters
    HKLOGINFO("[HKClient::sendTLVResponse] heap: %d\r\n", ESP.getFreeHeap());

    char response[headersSize + bodySize];
    HKLOGINFO("[HKClient::sendTLVResponse] alloc size: %d heap: %d\r\n", headersSize + bodySize, ESP.getFreeHeap());
    size_t currentSize = snprintf_P(response, headersSize, http_header_tlv8, bodySize);
    HKLOGINFO("[HKClient::sendTLVResponse] currentSize: %d strlen: %d heap: %d\r\n", currentSize, strlen(response), ESP.getFreeHeap());
    memcpy(response + currentSize, body, bodySize);
    HKLOGINFO("[HKClient::sendTLVResponse] bodySize: %d heap: %d\r\n", bodySize, ESP.getFreeHeap());

    send((uint8_t *) response, currentSize + bodySize);
    HKLOGINFO("[HKClient::sendTLVResponse] heap: %d\r\n", ESP.getFreeHeap());
}

void HKClient::sendTLVError(const uint8_t &state, const TLVError &error) {
    std::vector<HKTLV *> message = {
            new HKTLV(TLVTypeState, state, 1),
            new HKTLV(TLVTypeError, error, 1)
    };

    sendTLVResponse(message);
    for (auto it = message.begin(); it != message.end();) {
        delete *it;
        it = message.erase(it);
    }
}

void HKClient::processNotifications() {
    if (millis() - lastUpdate > NOTIFICATION_UPDATE_FREQUENCY && !events.empty()) {
        char header[sizeof(events_header_chunked)];
        strcpy_P(header, events_header_chunked);
        send((uint8_t *) header, sizeof(events_header_chunked)-1);

        JSON json = JSON(256, std::bind(&HKClient::sendChunk, this, std::placeholders::_1, std::placeholders::_2));
        json.startObject();
        json.setString("characteristics");
        json.startArray();

        for (HKEvent *event : events) {
            json.startObject();

            json.setString("aid");
            json.setInt(event->getCharacteristic()->getService()->getAccessory()->getId());

            event->getCharacteristic()->serializeToJSON(json, event->getValue(), 0);
            json.endObject();
        }

        json.endArray();
        json.endObject();

        json.flush();
        sendChunk(nullptr, 0);

        lastUpdate = millis();
    }
}

void HKClient::scheduleEvent(HKCharacteristic *characteristic, HKValue newValue) {
    events.push_back(new HKEvent(characteristic, newValue));
}

// PRIVATE FUNCTIONS

bool HKClient::decrypt(uint8_t *decryptedMessage, size_t &decryptedSize, const uint8_t *encryptedMessage, const size_t &encryptedSize) {
    if (!encrypted) {
        return false;
    }
    const size_t blockSize = 1024 + 16 + 2;
    decryptedSize = encryptedSize / blockSize * 1024 + encryptedSize % blockSize - 16 - 2;
    if (decryptedMessage == nullptr) {
        return true;
    }

    byte nonce[12];
    memset(nonce, 0, 12);

    size_t payloadOffset = 0;
    size_t decryptedOffset = 0;

    while (payloadOffset < encryptedSize) {
        size_t chunkSize = encryptedMessage[payloadOffset] + encryptedMessage[payloadOffset + 1]*256;
        if (chunkSize + 18 > encryptedSize - payloadOffset) {
            // Unfinished chunk
            break;
        }

        byte i = 4;
        int x = countWrites++;
        while (x) {
            nonce[i++] = x % 256;
            x /= 256;
        }

        size_t decryptedLen = decryptedSize - decryptedOffset;

        ChaChaPoly chaChaPoly = ChaChaPoly();
        chaChaPoly.setKey(writeKey, 32);
        chaChaPoly.setIV(nonce, 12);
        chaChaPoly.addAuthData(encryptedMessage + payloadOffset, 2);
        chaChaPoly.decrypt(decryptedMessage, encryptedMessage + payloadOffset + 2, chunkSize);
        if (!chaChaPoly.checkTag(encryptedMessage + payloadOffset + 2 + chunkSize, 16)) {
            HKLOGERROR("[HKClient::decrypt] Could not verify\r\n");
            return false;
        }

        decryptedOffset += decryptedLen;
        payloadOffset += chunkSize + 18;
    }
    
    return true;
}

void HKClient::sendEncrypted(byte *message, const size_t &messageSize) {
    if (!encrypted || !message || !messageSize) {
        return;
    }

    byte nonce[12];
    memset(nonce, 0, 12);

    size_t payloadOffset = 0;

    while (payloadOffset < messageSize) {
        size_t chunkSize = messageSize - payloadOffset;
        if (chunkSize > 1024) {
            chunkSize = 1024;
        }

        byte aead[2] = { static_cast<byte>(chunkSize % 256), static_cast<byte>(chunkSize / 256) };

        byte encryptedMessage[2 + chunkSize + 16];
        memcpy(encryptedMessage, aead, 2);

        byte i = 4;
        int x = countReads++;
        while (x) {
            nonce[i++] = x % 256;
            x /= 256;
        }

        ChaChaPoly chaChaPoly = ChaChaPoly();
        chaChaPoly.setKey(readKey, 32);
        chaChaPoly.setIV(nonce, 12);
        chaChaPoly.addAuthData(aead, 2);
        chaChaPoly.encrypt(encryptedMessage + 2, message + payloadOffset, chunkSize);
        chaChaPoly.computeTag(encryptedMessage + 2 + chunkSize, 16);
        payloadOffset += chunkSize;

        client.write(encryptedMessage, sizeof(encryptedMessage));
    }
}
