//
// Created by Max Vissing on 2019-06-07.
//

#include "HKClient.h"

HKClient::HKClient(HKServer *server, ClientContext* client) : WiFiClient(client), server(server), verifyContext(nullptr), encrypted(false), pairing(false), readKey(), countReads(0), writeKey(), countWrites(0), pairingId(0), permission(0) {}

HKClient::~HKClient() {
    delete verifyContext;
}

bool HKClient::readBytesWithTimeout(size_t maxLength, std::vector<byte> &data, int timeout_ms) {
    while (data.size() < maxLength) {
        int tries = timeout_ms;
        size_t avail;
        while (!(avail = available()) && tries--) {
            delay(1);
        }
        if (!avail) {
            break;
        }
        if (data.size() + avail > maxLength) {
            avail = maxLength - data.size();
        }
        while (avail--) {
            data.push_back(read());
        }
    }
    return data.size() == maxLength;
}

bool HKClient::received() {
    size_t dataSize;
    byte *data;
    size_t dataPos = 0;
    if (encrypted) {
        data = receivedDecrypted(dataSize);
    } else {
        dataSize = available();
        data = (byte *) malloc(dataSize);
        readBytes(data, dataSize);
    }

    String complete = String((char *) data);

    for (size_t i = dataPos; i < dataSize; i++) {
        if (data[i] == '\r') {
            data[i] = 0;
            break;
        }
    }
    String req = String((char *)data);
    dataPos += req.length() + 2;

    int addrStart = req.indexOf(' ');
    int addrEnd = req.indexOf(' ', addrStart + 1);
    if (addrStart == -1 || addrEnd == -1) {
        HKLOGWARNING("[HKClient::received] (ip=%s) Could not parse address (request=%s)\r\n", remoteIP().toString().c_str(), req.c_str());
        free(data);
        return false;
    }

    String methodStr = req.substring(0, addrStart);
    String url = req.substring(addrStart + 1, addrEnd);
    String versionEnd = req.substring(addrEnd + 8);
    // _currentVersion = atoi(versionEnd.c_str());
    String searchStr = "";
    int hasSearch = url.indexOf('?');
    if (hasSearch != -1) {
        searchStr = url.substring(hasSearch + 1);
        url = url.substring(0, hasSearch);
    }

    HTTPMethod method = HTTP_GET;
    if (methodStr == "POST") {
        method = HTTP_POST;
    } else if (methodStr == "PUT") {
        method = HTTP_PUT;
    }

    String formData;
    if (method == HTTP_POST || method == HTTP_PUT) {
        String headerName;
        String headerValue;
        String contentType;
        uint32_t contentLength = 0;

        while (true) {
            for (size_t i = dataPos; i < dataSize; i++) {
                if (data[i] == '\r') {
                    data[i] = 0;
                    break;
                }
            }
            req = String((char *)data + dataPos);
            dataPos += req.length() + 2;

            if (req == "") {
                break;
            }

            int headerDiv = req.indexOf(':');
            if (headerDiv == -1) {
                break;
            }
            headerName = req.substring(0, headerDiv);
            headerValue = req.substring(headerDiv + 1);
            headerValue.trim();

            if (headerName.equalsIgnoreCase("Content-Type")) {
                contentType = headerValue;
            } else if (headerName.equalsIgnoreCase("Content-Length")) {
                contentLength = headerValue.toInt();
            }
        }

        std::vector<byte> plainBuf;
        while (dataPos < dataSize) {
            plainBuf.push_back((byte) data[dataPos]);
            dataPos++;
        }

        if (!encrypted && plainBuf.size() < contentLength) {
            if (!readBytesWithTimeout(contentLength, plainBuf, 2000)) {
                HKLOGWARNING("[HKClient::received] (ip=%s) Timeout: %s\r\n", remoteIP().toString().c_str(), complete.c_str());
                return false;
            }
        }

        if (plainBuf.size() < contentLength) {
            HKLOGWARNING("[HKClient::received] (ip=%s) Data too short: %s\r\n", remoteIP().toString().c_str(), complete.c_str());

            free(data);
            return false;
        }

#if HKLOGLEVEL == 0
        HKLOGDEBUGSINGLE("------------- Received -------------\r\n");
        for (size_t i = 0; i < dataSize; i++) {
            byte item = *(data + i);
            if (item == '\r' || item == '\n' || (item >= ' ' && item <= '}' && item != '\\')) {
                HKLOGDEBUGSINGLE("%c", item);
            } else if (item < 0x10) {
                HKLOGDEBUGSINGLE("\\x0%x", item);
            } else {
                HKLOGDEBUGSINGLE("\\x%x", item);
            }
        }
        HKLOGDEBUGSINGLE("\r\n----------- End Received -----------\r\n");
#endif

        if (url == "/pair-setup") {
            onPairSetup(plainBuf);
        } else if (url == "/pair-verify") {
            onPairVerify(plainBuf);
        } else if (url == "/pairings") {
            onPairings(plainBuf);
        } else if (url == "/identify") {
            onIdentify();
        } else if (url == "/characteristics") {
            onUpdateCharacteristics(String((char *) plainBuf.data()));
        }
    } else {
        std::map<String, String> queries;
        while (searchStr.length()) {
            int equalPos = searchStr.indexOf('=');
            int keyEndPos = equalPos;
            int nextPos = searchStr.indexOf('&');
            int nextPos2 = searchStr.indexOf(';');
            if ((nextPos == -1) || (nextPos2 != -1 && nextPos2 < nextPos)) {
                nextPos = nextPos2;
            }
            if (nextPos == -1) {
                nextPos = searchStr.length();
            }
            if ((keyEndPos == -1) || (keyEndPos > nextPos && nextPos != -1)) {
                keyEndPos = nextPos;
            }
            if (keyEndPos == -1) {
                keyEndPos = searchStr.length();
            }

            String keyString = searchStr.substring(0, keyEndPos);
            String valueString;
            if ((uint) keyEndPos < searchStr.length() && keyEndPos < nextPos) {
                valueString = searchStr.substring(keyEndPos + 1, nextPos);
            }
            queries.insert(std::make_pair(keyString, valueString));

            searchStr = searchStr.substring(nextPos);
        }

#if HKLOGLEVEL == 0
        HKLOGDEBUGSINGLE("------------- Received -------------\r\n");
        for (size_t i = 0; i < dataSize; i++) {
            byte item = *(data + i);
            if (item == '\r' || item == '\n' || (item >= ' ' && item <= '}' && item != '\\')) {
                HKLOGDEBUGSINGLE("%c", item);
            } else if (item < 0x10) {
                HKLOGDEBUGSINGLE("\\x0%x", item);
            } else {
                HKLOGDEBUGSINGLE("\\x%x", item);
            }
        }
        HKLOGDEBUGSINGLE("\r\n----------- End Received -----------\r\n");
#endif

        if (url == "/accessories") {
            onGetAccessories();
        } else if (url == "/characteristics") {
            String id;
            if (queries.find("id") != queries.end()) {
                id = queries["id"];
            }
            bool meta = false;
            if (queries.find("meta") != queries.end()) {
                meta = queries["meta"] == "1";
            }
            bool perms = false;
            if (queries.find("perms") != queries.end()) {
                perms = queries["perms"] == "1";
            }
            bool type = false;
            if (queries.find("type") != queries.end()) {
                type = queries["type"] == "1";
            }
            bool ev = false;
            if (queries.find("ev") != queries.end()) {
                ev = queries["ev"] == "1";
            }
            onGetCharacteristics(id, meta, perms, type, ev);
        }
    }

    free(data);
    return true;
}

void HKClient::send(byte *message, size_t messageSize) {
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
        write(message, messageSize);
    }
}

void HKClient::sendChunk(byte *message, size_t messageSize) {
    size_t payloadSize = messageSize + 8;
    auto payload = (byte *) malloc(payloadSize);

    int offset = snprintf((char *) payload, payloadSize, "%x\r\n", messageSize);
    memcpy(payload + offset, message, messageSize);
    payload[offset + messageSize] = '\r';
    payload[offset + messageSize + 1] = '\n';

    send(payload, offset + messageSize + 2);

    free(payload);
}

byte *HKClient::receivedDecrypted(size_t &decryptedSize) {
    if (!encrypted) {
        return nullptr;
    }

    size_t encryptedSize = available();
    byte encryptedMessage[encryptedSize];
    readBytes(encryptedMessage, encryptedSize);

    const size_t blockSize = 1024 + 16 + 2;
    decryptedSize = encryptedSize / blockSize * 1024 + encryptedSize % blockSize - 16 - 2;
    auto decrypted = (byte *) malloc(decryptedSize);

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
        chaChaPoly.decrypt(decrypted, encryptedMessage + payloadOffset + 2, chunkSize);
        if (!chaChaPoly.checkTag(encryptedMessage + payloadOffset + 2 + chunkSize, 16)) {
            HKLOGERROR("[HKClient::receivedDecrypted] Could not verify\r\n");
            return nullptr;
        }

        decryptedOffset += decryptedLen;
        payloadOffset += chunkSize + 18;
    }

    return decrypted;
}

void HKClient::sendEncrypted(byte *message, size_t messageSize) {
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

        write(encryptedMessage, sizeof(encryptedMessage));
    }
}

void HKClient::sendTLVResponse(std::vector<HKTLV *> &message) {
    std::vector<byte> payload = HKTLV::formatTLV(message);
    String httpHeaders = F("HTTP/1.1 200 OK\r\n"
                         "Content-Type: application/pairing+tlv8\r\n"
                         "Content-Length: %d\r\n"
                         "Connection: keep-alive\r\n\r\n");
    httpHeaders.replace("%d", String(payload.size()));
    uint8_t response[httpHeaders.length() + payload.size()];

    memcpy(response, httpHeaders.c_str(), httpHeaders.length());
    memcpy(response + httpHeaders.length(), &payload[0], payload.size());

    send(response, httpHeaders.length() + payload.size());
}

void HKClient::sendTLVError(byte state, TLVError error) {
    std::vector<HKTLV *> message = {
            new HKTLV(TLVTypeState, state, 1),
            new HKTLV(TLVTypeError, error, 1)
    };

    sendTLVResponse(message);
}

void HKClient::hkdf(byte *target, byte *ikm, uint8_t ikmLength, byte *salt, uint8_t saltLength, byte *info,
                    uint8_t infoLength) {
    byte prk[64];
    SHA512 sha = SHA512();
    sha.resetHMAC(salt, saltLength);
    sha.update(ikm, ikmLength);
    sha.finalizeHMAC(salt, saltLength, prk, 64);

    sha.reset();
    sha.clear();
    sha.resetHMAC(prk, 64);
    sha.update(info, infoLength);
    sha.finalizeHMAC(prk, 64, target, 32);
}

void HKClient::onPairSetup(const std::vector<byte> &body) {
    std::vector<HKTLV *> message = HKTLV::parseTLV(body);

    uint32_t value = -1;
    HKTLV *state = HKTLV::findTLV(message, TLVTypeState);
    if (state) {
        value = state->getIntValue();
    }

    switch (value) {
        case 1: {
            HKLOGINFO("[HKClient::onPairSetup] Setup Step 1/3\r\n");
            if (HKStorage::isPaired()) {
                HKLOGINFO("[HKClient::onPairSetup] Refuse to pair: Already paired\r\n");

                sendTLVError(2, TLVErrorUnavailable);
                pairing = false;
                break;
            }

            if (server->isPairing()) {
                HKLOGINFO("[HKClient::onPairSetup] Refuse to pair: another pairing in process\r\n");

                sendTLVError(2, TLVErrorBusy);
                break;
            }
            pairing = true;

            server->hk->srp->start();

            for (auto it = message.begin(); it != message.end();) {
                delete *it;
                it = message.erase(it);
            }
            message.push_back(new HKTLV(TLVTypeState, 2, 1));
            message.push_back(new HKTLV(TLVTypePublicKey, server->hk->srp->getB(), 384));
            message.push_back(new HKTLV(TLVTypeSalt, server->hk->srp->getSalt(), 16));

            sendTLVResponse(message);
            break;
        }
        case 3: {
            HKLOGINFO("[HKClient::onPairSetup] Setup Step 2/3\r\n");
            HKTLV* publicKey = HKTLV::findTLV(message, TLVTypePublicKey);
            HKTLV* proof = HKTLV::findTLV(message, TLVTypeProof);
            if (!publicKey || !proof) {
                HKLOGINFO("[HKClient::onPairSetup] Could not find Public Key or Proof in Message\r\n");
                sendTLVError(4, TLVErrorAuthentication);
                pairing = false;
                break;
            }

            server->hk->srp->setA(publicKey->getValue(), publicKey->getSize(), nullptr);

            if (server->hk->srp->checkM1(proof->getValue(), proof->getSize())) {
                for (auto it = message.begin(); it != message.end();) {
                    delete *it;
                    it = message.erase(it);
                }
                message.push_back(new HKTLV(TLVTypeState, 4, 1));
                message.push_back(new HKTLV(TLVTypeProof, server->hk->srp->getM2(), 64));

                sendTLVResponse(message);
            } else {
                //return error
                HKLOGINFO("[HKClient::onPairSetup] SRP Error\r\n");
                sendTLVError(4, TLVErrorAuthentication);
                pairing = false;
            }
            break;
        }
        case 5: {
            HKLOGINFO("[HKClient::onPairSetup] Setup Step 3/3\r\n");

            byte sharedSecret[32];
            const char salt1[] = "Pair-Setup-Encrypt-Salt";
            const char info1[] = "Pair-Setup-Encrypt-Info\001";
            hkdf(sharedSecret, server->hk->srp->getK(), 64, (uint8_t *) salt1, sizeof(salt1)-1, (uint8_t *) info1, sizeof(info1)-1);

            HKTLV *encryptedTLV = HKTLV::findTLV(message, TLVTypeEncryptedData);
            if (!encryptedTLV) {
                HKLOGINFO("[HKClient::onPairSetup] Failed: Could not find Encrypted Data\r\n");
                sendTLVError(6, TLVErrorAuthentication);
                pairing = false;
                break;
            }

            size_t decryptedDataSize = encryptedTLV->getSize() - 16;
            byte decryptedData[decryptedDataSize];

            if (!crypto_verifyAndDecrypt(sharedSecret, (byte *) "PS-Msg05", encryptedTLV->getValue(), decryptedDataSize, decryptedData, encryptedTLV->getValue() + decryptedDataSize)) {
                HKLOGINFO("[HKClient::onPairSetup] Decryption failed: MAC not equal\r\n");
                sendTLVError(6, TLVErrorAuthentication);
                pairing = false;
                break;
            }

            std::vector<HKTLV *> decryptedMessage = HKTLV::parseTLV(decryptedData, decryptedDataSize);
            HKTLV *deviceId = HKTLV::findTLV(decryptedMessage, TLVTypeIdentifier);
            if (!deviceId) {
                HKLOGINFO("[HKClient::onPairSetup] Decryption failed: Device ID not found in decrypted Message\r\n");
                for (auto msg : decryptedMessage) {
                    delete msg;
                }
                sendTLVError(6, TLVErrorAuthentication);
                pairing = false;
                break;
            }

            HKTLV *publicKey = HKTLV::findTLV(decryptedMessage, TLVTypePublicKey);
            if (!publicKey) {
                HKLOGINFO("[HKClient::onPairSetup] Decryption failed: Public Key not found in decrypted Message\r\n");
                for (auto msg : decryptedMessage) {
                    delete msg;
                }
                sendTLVError(6, TLVErrorAuthentication);
                pairing = false;
                break;
            }

            HKTLV *signature = HKTLV::findTLV(decryptedMessage, TLVTypeSignature);
            if (!signature) {
                HKLOGINFO("[HKClient::onPairSetup] Decryption failed: Signature not found in decrypted Message\r\n");
                for (auto msg : decryptedMessage) {
                    delete msg;
                }
                sendTLVError(6, TLVErrorAuthentication);
                pairing = false;
                break;
            }

            byte deviceX[32];
            const char salt2[] = "Pair-Setup-Controller-Sign-Salt";
            const char info2[] = "Pair-Setup-Controller-Sign-Info\001";
            hkdf(deviceX, server->hk->srp->getK(), 64, (uint8_t *) salt2, sizeof(salt2)-1, (uint8_t *) info2, sizeof(info2)-1);

            uint64_t deviceInfoSize = sizeof(deviceX) + deviceId->getSize() + publicKey->getSize();
            byte deviceInfo[deviceInfoSize];
            memcpy(deviceInfo, deviceX, sizeof(deviceX));
            memcpy(deviceInfo + sizeof(deviceX), deviceId->getValue(), deviceId->getSize());
            memcpy(deviceInfo + sizeof(deviceX) + deviceId->getSize(), publicKey->getValue(), publicKey->getSize());

            if (!Ed25519::verify(signature->getValue(), publicKey->getValue(), deviceInfo, deviceInfoSize)) {
                HKLOGINFO("[HKClient::onPairSetup] Could not verify Ed25519 Device Info, Signature and Public Key\r\n");
                for (auto msg : decryptedMessage) {
                    delete msg;
                }
                sendTLVError(6, TLVErrorAuthentication);
                pairing = false;
                break;
            }

            int result = HKStorage::addPairing((char *) deviceId->getValue(), publicKey->getValue(), 1);
            if (result) {
                HKLOGERROR("[HKClient::onPairSetup] COULD NOT STORE PAIRING\r\n");
            }

            // M6 Response Generation
            String accessoryId = HKStorage::getAccessoryId();
            size_t accessoryInfoSize = 32 + accessoryId.length() + 32;
            byte accessoryInfo[accessoryInfoSize];
            const char salt3[] = "Pair-Setup-Accessory-Sign-Salt";
            const char info3[] = "Pair-Setup-Accessory-Sign-Info\001";
            hkdf(accessoryInfo, server->hk->srp->getK(), 64, (uint8_t *) salt3, sizeof(salt3)-1, (uint8_t *) info3, sizeof(info3)-1);
            delete server->hk->srp;

            memcpy(accessoryInfo + 32, accessoryId.c_str(), accessoryId.length());
            memcpy(accessoryInfo + 32 + accessoryId.length(), HKStorage::getAccessoryKey().publicKey, 32);

            byte accessorySignature[64];
            Ed25519::sign(accessorySignature, HKStorage::getAccessoryKey().privateKey, HKStorage::getAccessoryKey().publicKey, accessoryInfo, accessoryInfoSize);

            std::vector<HKTLV *> responseMessage = {
                    new HKTLV(TLVTypeIdentifier, (byte *) accessoryId.c_str(), accessoryId.length()),
                    new HKTLV(TLVTypePublicKey, HKStorage::getAccessoryKey().publicKey, 32),
                    new HKTLV(TLVTypeSignature, accessorySignature, 64)
            };
            std::vector<byte> responseData = HKTLV::formatTLV(responseMessage);

            byte encryptedResponseData[responseData.size() + 16];
            crypto_encryptAndSeal(sharedSecret, (uint8_t *) "PS-Msg06", responseData.data(), responseData.size(), encryptedResponseData, encryptedResponseData + responseData.size());

            for (auto msg : decryptedMessage) {
                delete msg;
            }
            for (auto it = message.begin(); it != message.end();) {
                delete *it;
                it = message.erase(it);
            }
            message.push_back(new HKTLV(TLVTypeState, 6, 1));
            message.push_back(new HKTLV(TLVTypeEncryptedData, encryptedResponseData, responseData.size() + 16));

            sendTLVResponse(message);

            server->setupMDNS();

            pairing = false;
            HKLOGINFO("[HKClient::onPairSetup] Pairing Successfull\r\n");
            break;
        }
        default:
            break;
    }

    for (auto msg : message) {
        delete msg;
    }
}

void HKClient::onPairVerify(const std::vector<byte> &body) {
    std::vector<HKTLV *> message = HKTLV::parseTLV(body);

    int value = -1;
    HKTLV *state = HKTLV::findTLV(message, TLVTypeState);
    if (state) {
        value = state->getIntValue();
    }

    switch (value) {
        case 1: {
            HKLOGINFO("[HKClient::onPairVerify] Verify Step 1/2\r\n");
            verifyContext = new VerifyContext();

            HKTLV *deviceKeyTLV = HKTLV::findTLV(message, TLVTypePublicKey);

            if (!deviceKeyTLV) {
                HKLOGINFO("[HKClient::onPairVerify] Device Key not Found\r\n");
                sendTLVError(2, TLVErrorUnknown);
                break;
            }
            memcpy(verifyContext->devicePublicKey, deviceKeyTLV->getValue(), 32);

            os_get_random(verifyContext->accessorySecretKey, 32);

            crypto_scalarmult_curve25519_base(verifyContext->accessoryPublicKey, verifyContext->accessorySecretKey);
            crypto_scalarmult_curve25519(verifyContext->sharedKey, verifyContext->accessorySecretKey, verifyContext->devicePublicKey);

            String accessoryId = HKStorage::getAccessoryId();
            size_t accessoryInfoSize = 32 + accessoryId.length() + 32;
            byte accessoryInfo[accessoryInfoSize];
            memcpy(accessoryInfo, verifyContext->accessoryPublicKey, 32);
            memcpy(accessoryInfo + 32, accessoryId.c_str(), accessoryId.length());
            memcpy(accessoryInfo + 32 + accessoryId.length(), verifyContext->devicePublicKey, 32);

            byte accessorySignature[64];
            Ed25519::sign(accessorySignature, HKStorage::getAccessoryKey().privateKey, HKStorage::getAccessoryKey().publicKey, accessoryInfo, accessoryInfoSize);

            std::vector<HKTLV *> subResponseMessage = {
                    new HKTLV(TLVTypeIdentifier, (byte *) accessoryId.c_str(), accessoryId.length()),
                    new HKTLV(TLVTypeSignature, accessorySignature, 64)
            };
            std::vector<byte> responseData = HKTLV::formatTLV(subResponseMessage);

            const char salt1[] = "Pair-Verify-Encrypt-Salt";
            const char info1[] = "Pair-Verify-Encrypt-Info\001";
            hkdf(verifyContext->sessionKey, verifyContext->sharedKey, 32, (byte *) salt1, sizeof(salt1)-1, (byte *) info1, sizeof(info1)-1);

            byte encryptedResponseData[responseData.size() + 16];
            crypto_encryptAndSeal(verifyContext->sessionKey, (byte *) "PV-Msg02", responseData.data(), responseData.size(), encryptedResponseData, encryptedResponseData + responseData.size());

            std::vector<HKTLV *> responseMessage = {
                    new HKTLV(TLVTypeState, 2, 1),
                    new HKTLV(TLVTypePublicKey, verifyContext->accessoryPublicKey, 32),
                    new HKTLV(TLVTypeEncryptedData, encryptedResponseData, responseData.size() + 16)
            };

            sendTLVResponse(responseMessage);
            break;
        }
        case 3: {
            HKLOGINFO("[HKClient::onPairVerify] Verify Step 2/2\r\n");

            if (!verifyContext) {
                sendTLVError(4, TLVErrorAuthentication);
                break;
            }

            HKTLV *encryptedData = HKTLV::findTLV(message, TLVTypeEncryptedData);
            if (!encryptedData) {
                HKLOGINFO("[HKClient::onPairVerify] Could not find encrypted data\r\n");
                sendTLVError(4, TLVErrorUnknown);
                delete verifyContext;
                verifyContext = nullptr;
                break;
            }

            size_t decryptedDataSize = encryptedData->getSize() - 16;
            byte decryptedData[decryptedDataSize];
            if (!crypto_verifyAndDecrypt(verifyContext->sessionKey, (byte *) "PV-Msg03", encryptedData->getValue(), decryptedDataSize, decryptedData, encryptedData->getValue() + decryptedDataSize)) {
                HKLOGINFO("[HKClient::onPairVerify] Could not verify message\r\n");
                sendTLVError(4, TLVErrorAuthentication);
                delete verifyContext;
                verifyContext = nullptr;
                break;
            }

            std::vector<HKTLV *> decryptedMessage = HKTLV::parseTLV(decryptedData, decryptedDataSize);
            HKTLV *deviceId = HKTLV::findTLV(decryptedMessage, TLVTypeIdentifier);
            if (!deviceId) {
                HKLOGINFO("[HKClient::onPairVerify] Could not find device ID\r\n");
                for (auto msg : decryptedMessage) {
                    delete msg;
                }
                sendTLVError(4, TLVErrorAuthentication);
                delete verifyContext;
                verifyContext = nullptr;
                break;
            }

            HKTLV *deviceSignature = HKTLV::findTLV(decryptedMessage, TLVTypeSignature);
            if (!deviceSignature) {
                HKLOGINFO("[HKClient::onPairVerify] Could not find device Signature\r\n");
                for (auto msg : decryptedMessage) {
                    delete msg;
                }
                sendTLVError(4, TLVErrorAuthentication);
                delete verifyContext;
                verifyContext = nullptr;
                break;
            }

            Pairing *pairingItem = HKStorage::findPairing((char *) deviceId->getValue());
            if (!pairingItem) {
                HKLOGINFO("[HKClient::onPairVerify] Device is not paired\r\n");
                for (auto msg : decryptedMessage) {
                    delete msg;
                }
                sendTLVError(4, TLVErrorAuthentication);
                delete verifyContext;
                verifyContext = nullptr;
                break;
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
                sendTLVError(4, TLVErrorAuthentication);
                delete verifyContext;
                verifyContext = nullptr;
                break;
            }

            const byte salt[] = "Control-Salt";
            const byte readInfo[] = "Control-Read-Encryption-Key\001";
            hkdf(readKey, verifyContext->sharedKey, 32, (uint8_t *) salt, sizeof(salt)-1, (uint8_t *) readInfo, sizeof(readInfo)-1);

            const byte writeInfo[] = "Control-Write-Encryption-Key\001";
            hkdf(writeKey, verifyContext->sharedKey, 32, (uint8_t *) salt, sizeof(salt)-1, (uint8_t *) writeInfo, sizeof(writeInfo)-1);

            pairingId = pairingItem->id;
            permission = pairingItem->permissions;
            delete pairingItem;

            std::vector<HKTLV *> responseMessage = {
                    new HKTLV(TLVTypeState, 4, 1)
            };
            sendTLVResponse(responseMessage);

            for (auto msg : decryptedMessage) {
                delete msg;
            }
            delete verifyContext;
            verifyContext = nullptr;
            encrypted = true;
            break;
        }
        default:
            break;
    }

    for (auto msg : message) {
        delete msg;
    }
}

void HKClient::onIdentify() {
    HKLOGINFO("[HKClient::onIdentify] Identify\r\n");

    if (HKStorage::isPaired()) {
        sendJSONErrorResponse(400, HAPStatusInsufficientPrivileges);
        return;
    }

    send204Response();

    HKAccessory *accessory = server->hk->getAccessory();
    if (!accessory) {
        return;
    }

    HKService *accessoryInfo = accessory->getService(HKServiceAccessoryInfo);
    if (!accessoryInfo) {
        return;
    }

    HKCharacteristic *characteristicIdentify = accessoryInfo->getCharacteristic(HKCharacteristicIdentify);
    if (!characteristicIdentify) {
        return;
    }

    if (characteristicIdentify->setter) {
        characteristicIdentify->setter(HKValue(FormatBool, true));
    }
}

void HKClient::onGetAccessories() {
    HKLOGINFO("[HKClient::onGetAccessories] Get Accessories\r\n");

    send((byte *) json_200_response_headers, sizeof(json_200_response_headers) - 1);

    JSON json = JSON(1024, std::bind(&HKClient::sendChunk, this, std::placeholders::_1, std::placeholders::_2));
    json.startObject();
    json.setString("accessories");
    json.startArray();

    HKAccessory *accessory = server->hk->getAccessory();
    accessory->serializeToJSON(json, nullptr, this);

    json.endArray();

    json.endObject();

    json.flush();

    sendChunk(nullptr, 0);
}

void HKClient::onGetCharacteristics(String id, bool meta, bool perms, bool type, bool ev) {
    HKLOGINFO("[HKClient::onGetCharacteristics] Get Characteristics\r\n");

    if (!id) {
        sendJSONErrorResponse(400, HAPStatusInvalidValue);
        return;
    }

    uint format = 0;
    if (meta) {
        format |= HKCharacteristicFormatMeta;
    }
    if (perms) {
        format |= HKCharacteristicFormatPerms;
    }
    if (type) {
        format |= HKCharacteristicFormatType;
    }
    if (ev) {
        format |= HKCharacteristicFormatEvents;
    }

    bool success = true;

    String idCpy = id;
    while (idCpy.length() > 0) {
        int aidPos = idCpy.indexOf('.');
        if (aidPos == -1) {
            sendJSONErrorResponse(400, HAPStatusInvalidValue);
            return;
        }
        int iidPos = idCpy.indexOf(',');
        if (iidPos == -1) {
            iidPos = idCpy.length();
        }

        uint aid = idCpy.substring(0, aidPos).toInt();
        uint iid = idCpy.substring(aidPos + 1, iidPos).toInt();
        idCpy = idCpy.substring(iidPos + 1);

        if (server->hk->getAccessory()->getId() == aid) {
            if (HKCharacteristic* target = server->hk->getAccessory()->findCharacteristic(iid)) {
                if (!(target->permissions & PermissionPairedRead)) {
                    success = false;
                }
            } else {
                HKLOGWARNING("[HKClient::onGetCharacteristics] Could not find characteristic with id=%d.%d\r\n", aid, iid);
                success = false;
            }
        } else {
            HKLOGWARNING("[HKClient::onGetCharacteristics] Could not find accessory with id=%d\r\n", aid);
            success = false;
        }
    }

    if (success) {
        send((byte *) json_200_response_headers, sizeof(json_200_response_headers) - 1);
    } else {
        send((byte *) json_207_response_headers, sizeof(json_207_response_headers) - 1);
    }

    JSON json = JSON(1024, std::bind(&HKClient::sendChunk, this, std::placeholders::_1, std::placeholders::_2));
    json.startObject();
    json.setString("characteristics");
    json.startArray();

    while (id.length() > 0) {
        int aidPos = id.indexOf('.');
        int iidPos = id.indexOf(',');
        if (iidPos == -1) {
            iidPos = id.length();
        }

        uint aid = id.substring(0, aidPos).toInt();
        uint iid = id.substring(aidPos + 1, iidPos).toInt();
        id = id.substring(iidPos + 1);

        if (server->hk->getAccessory()->getId() == aid) {
            if (HKCharacteristic* target = server->hk->getAccessory()->findCharacteristic(iid)) {

                json.startObject();

                json.setString("aid");
                json.setInt(aid);

                if (!(target->permissions & PermissionPairedRead)) {
                    json.setString("iid");
                    json.setInt(iid);
                    json.setString("status");
                    json.setInt(HAPStatusWriteOnly);
                    json.endObject();
                    continue;
                }

                target->serializeToJSON(json, nullptr, format, this);

                if (!success) {
                    json.setString("status");
                    json.setInt(HAPStatusSuccess);
                }

                json.endObject();
            } else {
                json.startObject();
                json.setString("aid");
                json.setInt(aid);
                json.setString("iid");
                json.setInt(iid);
                json.setString("status");
                json.setInt(HAPStatusNoResource);
                json.endObject();
            }
        } else {
            json.startObject();
            json.setString("aid");
            json.setInt(aid);
            json.setString("iid");
            json.setInt(iid);
            json.setString("status");
            json.setInt(HAPStatusNoResource);
            json.endObject();
        }
    }

    json.endArray();

    json.endObject();

    json.flush();

    sendChunk(nullptr, 0);
}

void HKClient::onUpdateCharacteristics(const String &jsonBody) {
    HKLOGINFO("[HKClient::onUpdateCharacteristics] Update Characteristics\r\n");

    HKLOGDEBUG("jsonBody begin: %s\r\n", jsonBody.substring(0, 20).c_str());
    if (jsonBody.substring(0, 20) != "{\"characteristics\":[") {
        HKLOGERROR("[HKClient::onUpdateCharacteristics] Could not deserialize json beginning not equal\r\n");
        sendJSONErrorResponse(400, HAPStatusInvalidValue);
        return;
    }

    uint cursor = 20;
    while (jsonBody.length() > cursor) {
        int begin = jsonBody.indexOf('{', cursor);
        int end = jsonBody.indexOf('}', cursor);
        if (begin == -1 || end == -1) {
            break;
        }
        String item = jsonBody.substring(begin + 1, end);

        HKLOGDEBUG("jsonBody item: %s\r\n", item.c_str());
        int aidPos = item.indexOf("\"aid\"");
        int iidPos = item.indexOf("\"iid\"");
        int evPos = item.indexOf("\"ev\"");
        int valuePos = item.indexOf("\"value\"");
        HKLOGDEBUG("jsonBody aidPos: %d iidPos: %d evPos: %d valuePos: %d\r\n", aidPos, iidPos, evPos, valuePos);
        if (aidPos == -1 || iidPos == -1 || (evPos == -1 && valuePos == -1)) {
            break;
        }

        int aidBeginPos = item.indexOf(':', aidPos);
        int aidEndPos = item.indexOf(',', aidPos);
        int iidBeginPos = item.indexOf(':', iidPos);
        int iidEndPos = item.indexOf(',', iidPos);
        HKLOGDEBUG("jsonBody aidBeginPos: %d aidEndPos: %d iidBeginPos: %d iidEndPos: %d\r\n", aidBeginPos, aidEndPos, iidBeginPos, iidEndPos);
        if (aidBeginPos == -1 || aidEndPos == -1 || iidBeginPos == -1 || iidEndPos == -1) {
            break;
        }

        int aid = item.substring(aidBeginPos + 1, aidEndPos).toInt();
        int iid = item.substring(iidBeginPos + 1, iidEndPos).toInt();
        HKLOGDEBUG("jsonBody aid: %d iid: %d\r\n", aid, iid);
        if (aid == 0 || iid == 0) {
            break;
        }

        String ev;
        String value;
        if (evPos != -1) {
            int evBeginPos = item.indexOf(':', evPos);
            int evEndPosSeparator = item.indexOf(',', evPos);
            int evEndPosEnd = item.indexOf('}', evPos);
            int evEndPos;
            if (evEndPosEnd == -1) {
                evEndPosEnd = item.length();
            }
            if (evEndPosSeparator == -1 || evEndPosSeparator > evEndPosEnd) {
                evEndPos = evEndPosEnd;
            } else {
                evEndPos = evEndPosSeparator;
            }
            if (evBeginPos != -1 && evEndPos != -1) {
                ev = item.substring(evBeginPos + 1, evEndPos);
                ev.trim();
            }
        }
        if (valuePos != -1) {
            int valueBeginPos = item.indexOf(':', valuePos);
            int valueEndPosSeparator = item.indexOf(',', valuePos);
            int valueEndPosEnd = item.indexOf('}', valuePos);
            int valueEndPos;
            if (valueEndPosEnd == -1) {
                valueEndPosEnd = item.length();
            }
            if (valueEndPosSeparator == -1 || valueEndPosSeparator > valueEndPosEnd) {
                valueEndPos = valueEndPosEnd;
            } else {
                valueEndPos = valueEndPosSeparator;
            }
            if (valueBeginPos != -1 && valueEndPos != -1) {
                value = item.substring(valueBeginPos + 1, valueEndPos);
                value.trim();
            }
        }

        HKLOGDEBUG("jsonBody ev: %s value: %s\r\n", ev.c_str(), value.c_str());
        processUpdateCharacteristic(aid, iid, ev, value);

        cursor = end + 1;
    }

    send204Response();
}

HAPStatus HKClient::processUpdateCharacteristic(uint aid, uint iid, String ev, String value) {
    HKAccessory *accessory = server->hk->getAccessory();
    if (accessory->getId() != aid) {
        HKLOGWARNING("[HKClient::processUpdateCharacteristic] Could not find accessory with id=%d\r\n", aid);
        return HAPStatusNoResource;
    }

    HKCharacteristic *characteristic = accessory->findCharacteristic(iid);
    if (!characteristic) {
        HKLOGWARNING("[HKClient::processUpdateCharacteristic] Could not find characteristic with id=%d.%d\r\n", aid, iid);
        return HAPStatusNoResource;
    }

    HAPStatus status = HAPStatusSuccess;
    if (value.length() > 0) {
        status = characteristic->setValue(value);
        if (status != HAPStatusSuccess) {
            return status;
        }
    }

    if (ev.length() > 0) {
        status = characteristic->setEvent(this, ev);
    }
    return status;
}

void HKClient::onPairings(const std::vector<byte> &body) {
    HKLOGINFO("[HKClient::onPairings] Pairings\r\n");

    std::vector<HKTLV *> message = HKTLV::parseTLV(body);
    HKTLV *state = HKTLV::findTLV(message, TLVTypeState);
    if (!state || state->getIntValue() != 1) {
        sendTLVError(2, TLVErrorUnknown);

        for (auto msg : message) {
            delete msg;
        }
        HKLOGERROR("[HKClient::onPairings] Unknown State\r\n");
        return;
    }

    HKTLV *method = HKTLV::findTLV(message, TLVTypeMethod);
    if (!method) {
        sendTLVError(2, TLVErrorUnknown);

        for (auto msg : message) {
            delete msg;
        }
        HKLOGERROR("[HKClient::onPairings] Unknown Message\r\n");
        return;
    }
    switch ((TLVMethod) method->getIntValue()) {
        case TLVMethodPairSetup:
            onPairSetup(body);
            break;
        case TLVMethodPairVerify:
            onPairVerify(body);
            break;
        case TLVMethodAddPairing: {
            HKLOGINFO("[HKClient::onPairings] Add Pairing\r\n");
            if (!(permission & PairingPermissionAdmin)) {
                HKLOGWARNING("[HKClient::onPairings] Refusing to add pairing to non-admin controller\r\n");
                sendTLVError(2, TLVErrorAuthentication);
                break;
            }

            HKTLV *deviceId = HKTLV::findTLV(message, TLVTypeIdentifier);
            if (!deviceId) {
                HKLOGWARNING("[HKClient::onPairings] Invalid add pairing request: no device identifier\r\n");
                sendTLVError(2, TLVErrorUnknown);
                break;
            }

            HKTLV *devicePublicKey = HKTLV::findTLV(message, TLVTypePublicKey);
            if (!devicePublicKey) {
                HKLOGWARNING("[HKClient::onPairings] Invalid add pairing request: no device public key\r\n");
                sendTLVError(2, TLVErrorUnknown);
                break;
            }

            HKTLV *devicePermission = HKTLV::findTLV(message, TLVTypePermissions);
            if (!devicePermission) {
                HKLOGWARNING("[HKClient::onPairings] Invalid add pairing request: no device Permissions\r\n");
                sendTLVError(2, TLVErrorUnknown);
                break;
            }

            char *deviceIdentifier = strndup((const char *) deviceId->getValue(), deviceId->getSize());
            Pairing *comparePairing = HKStorage::findPairing(deviceIdentifier);
            if (comparePairing) {
                if (devicePublicKey->getSize() != 32 || memcmp(devicePublicKey->getValue(), comparePairing->deviceKey, 32) != 0) {
                    HKLOGWARNING("[HKClient::onPairings] Failed to add pairing: pairing public key differs from given one\r\n");
                    delete comparePairing;
                    free(deviceIdentifier);
                    sendTLVError(2, TLVErrorUnknown);
                    break;
                }
                delete comparePairing;

                if (HKStorage::updatePairing(deviceIdentifier, *devicePermission->getValue())) {
                    HKLOGWARNING("[HKClient::onPairings] Failed to add pairing: storage error\r\n");
                    free(deviceIdentifier);
                    sendTLVError(2, TLVErrorUnknown);
                    break;
                }

                HKLOGINFO("[HKClient::onPairings] Updated pairing with id=%s\r\n", deviceIdentifier);
            } else {
                int r = HKStorage::addPairing(deviceIdentifier, devicePublicKey->getValue(), *devicePermission->getValue());
                if (r == -2) {
                    HKLOGWARNING("[HKClient::onPairings] Failed to add pairing: max peers\r\n");
                    free(deviceIdentifier);
                    sendTLVError(2, TLVErrorMaxPeers);
                    break;
                } else if (r != 0) {
                    HKLOGWARNING("[HKClient::onPairings] Failed to add pairing: Storage error\r\n");
                    free(deviceIdentifier);
                    sendTLVError(2, TLVErrorUnknown);
                    break;
                }

                delete comparePairing;

                HKLOGINFO("[HKClient::onPairings] Added pairing with id=%s\r\n", deviceIdentifier);
            }
            free(deviceIdentifier);

            std::vector<HKTLV *> response = {
                new HKTLV(TLVTypeState, 2, 1),
            };
            sendTLVResponse(response);
            break;
        }
        case TLVMethodRemovePairing: {
            HKLOGINFO("[HKClient::onPairings] Remove pairing\r\n");

            if (!(permission & PairingPermissionAdmin)) {
                HKLOGWARNING("[HKClient::onPairings] Refuse to remove pairing to non-admin controller\r\n");
                sendTLVError(2, TLVErrorAuthentication);
                break;
            }

            HKTLV *deviceId = HKTLV::findTLV(message, TLVTypeIdentifier);
            if (!deviceId) {
                HKLOGWARNING("[HKClient::onPairings] Invalid remove pairing request: no device identifier\r\n");
                sendTLVError(2, TLVErrorUnknown);
                break;
            }

            char *deviceIdentifier = strndup((const char *) deviceId->getValue(), deviceId->getSize());
            Pairing *comparePairing = HKStorage::findPairing(deviceIdentifier);
            if (comparePairing) {
                bool isAdmin = comparePairing->permissions & PairingPermissionAdmin;

                int result = HKStorage::removePairing(deviceIdentifier);
                if (result) {
                    delete comparePairing;
                    free(deviceIdentifier);
                    HKLOGERROR("[HKClient::onPairings] Failed to remove pairing: storage error\r\n");
                    sendTLVError(2, TLVErrorUnknown);
                    break;
                }

                HKLOGINFO("[HKClient::onPairings] Removed pairing with id=%s\r\n", deviceIdentifier);
#if HKLOGLEVEL <= 1
                for (auto pPairing : HKStorage::getPairings()) {
                    HKLOGINFO("[HKStorage] Pairing id=%i deviceId=%36s permissions=%i\r\n", pPairing->id, pPairing->deviceId, pPairing->permissions);
                }
#endif

                for (auto client : server->clients) {
                    if (client->pairingId == comparePairing->id) {
                        client->stop();
                    }
                }

                if (isAdmin) {
                    if (!HKStorage::hasPairedAdmin()) {
                        HKLOGINFO("[HKClient::onPairings] Last admin pairing was removed, enabling pair setup\r\n");
                        server->setupMDNS();
                    }
                }
            }
            free(deviceIdentifier);

            std::vector<HKTLV *> response = {
                    new HKTLV(TLVTypeState, 2, 1)
            };
            sendTLVResponse(response);
            break;
        }
        case TLVMethodListPairings: {
            if (!(permission & PairingPermissionAdmin)) {
                HKLOGWARNING("[HKClient::onPairings] Refusing to list pairings to non-admin controller\r\n");
                sendTLVError(2, TLVErrorAuthentication);
                break;
            }

            std::vector<HKTLV *> response = {
                    new HKTLV(TLVTypeState, 2, 1),
            };

            bool first = true;
            std::vector<Pairing *> pairings = HKStorage::getPairings();
            for (auto pairingItem : pairings) {
                if (!first) {
                    response.push_back(new HKTLV(TLVTypeSeparator, nullptr, 0));
                }
                first = false;

                response.push_back(new HKTLV(TLVTypeIdentifier, (byte *) pairingItem->deviceId, 36));
                response.push_back(new HKTLV(TLVTypePublicKey, pairingItem->deviceKey, 32));
                response.push_back(new HKTLV(TLVTypePermissions, pairingItem->permissions, 1));
            }

            sendTLVResponse(response);

            for (auto pairingItem : pairings) {
                delete pairingItem;
            }

            break;
        }
    }

    for (auto msg : message) {
        delete msg;
    }
}

void HKClient::send204Response() {
    static char response[] = "HTTP/1.1 204 No Content\r\n\r\n";
    send((byte *)response, sizeof(response)-1);
}

void HKClient::scheduleEvent(HKCharacteristic *characteristic, HKValue newValue) {
    events.push_back(new HKEvent(characteristic, newValue));
}

void HKClient::sendEvents(ClientEvent *event) {
    HKLOGINFO("[HKClient::sendEvents] sending Event\r\n");
    static byte http_headers[] = "EVENT/1.0 200 OK\r\n"
                                 "Content-Type: application/hap+json\r\n"
                                 "Transfer-Encoding: chunked\r\n\r\n";

    send(http_headers, sizeof(http_headers)-1);

    JSON json = JSON(256, std::bind(&HKClient::sendChunk, this, std::placeholders::_1, std::placeholders::_2));
    json.startObject();
    json.setString("characteristics");
    json.startArray();

    ClientEvent *e = event;
    while (e) {
        json.startObject();

        json.setString("aid");
        json.setInt(e->characteristic->service->getAccessory()->getId());

        e->characteristic->serializeToJSON(json, &e->value, 0);
        json.endObject();
        e = e->next;
    }

    json.endArray();
    json.endObject();

    json.flush();
    sendChunk(nullptr, 0);
}

void HKClient::sendJSONErrorResponse(int errorCode, HAPStatus status) {
    String message = "{\"status\": " + String(status) + "}";
    sendJSONResponse(errorCode, message);
}

void HKClient::sendJSONResponse(int errorCode, const String& message) {
    String statusText;
    switch (errorCode) {
        case 204: statusText = "No Content"; break;
        case 207: statusText = "Multi-Status"; break;
        case 400: statusText = "Bad Request"; break;
        case 404: statusText = "Not Found"; break;
        case 422: statusText = "Unprocessable Entity"; break;
        case 500: statusText = "Internal Server Error"; break;
        case 503: statusText = "Service Unavailable"; break;
        default: statusText = "OK"; break;
    }
    String response = "HTTP/1.1 " + String(errorCode) + " " + statusText + "\r\n"
            "Content-Type: application/hap+json\r\n"
            "Content-Length: " + String(message.length()) + "\r\n"
            "Connection: keep-alive\r\n\r\n" + message;
    send((byte *) response.c_str(), response.length());
}
