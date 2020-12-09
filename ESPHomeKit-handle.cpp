
#include "ESPHomeKit.h"

void ESPHomeKit::onGetAccessories(HKClient *client) {
    HKLOGINFO("[HKClient::onGetAccessories] Get Accessories\r\n");

    char *header = (char *) malloc(sizeof(http_header_200_chunked));
    strcpy_P(header, http_header_200_chunked);
    client->send((uint8_t *) header, sizeof(http_header_200_chunked)-1);
    free(header);

    JSON json = JSON(1024, std::bind(&HKClient::sendChunk, client, std::placeholders::_1, std::placeholders::_2));
    json.startObject();
    json.setString("accessories");
    json.startArray();

    accessory->serializeToJSON(json, nullptr, client);

    json.endArray();
    json.endObject();
    json.flush();

    client->sendChunk(nullptr, 0);
}

void ESPHomeKit::onGetCharacteristics(HKClient *client, String id, bool meta, bool perms, bool type, bool ev) {
    HKLOGINFO("[HKClient::onGetCharacteristics] Get Characteristics\r\n");

    if (!id) {
        client->sendJSONErrorResponse(400, HAPStatusInvalidValue);
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
            client->sendJSONErrorResponse(400, HAPStatusInvalidValue);
            return;
        }
        int iidPos = idCpy.indexOf(',');
        if (iidPos == -1) {
            iidPos = idCpy.length();
        }

        uint aid = idCpy.substring(0, aidPos).toInt();
        uint iid = idCpy.substring(aidPos + 1, iidPos).toInt();
        idCpy = idCpy.substring(iidPos + 1);

        if (accessory->getId() == aid) {
            if (HKCharacteristic* target = accessory->findCharacteristic(iid)) {
                if (!(target->permissions & HKPermissionPairedRead)) {
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
        char *response = (char *) malloc(sizeof(http_header_200_chunked));
        strcpy_P(response, http_header_200_chunked);
        client->send((uint8_t *) response, sizeof(http_header_200_chunked)-1);
        free(response);
    } else {
        char *response = (char *) malloc(sizeof(http_header_207_chunked));
        strcpy_P(response, http_header_207_chunked);
        client->send((uint8_t *) response, sizeof(http_header_207_chunked)-1);
        free(response);
    }

    JSON json = JSON(1024, std::bind(&HKClient::sendChunk, client, std::placeholders::_1, std::placeholders::_2));
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

        if (accessory->getId() == aid) {
            if (HKCharacteristic* target = accessory->findCharacteristic(iid)) {

                json.startObject();

                json.setString("aid");
                json.setInt(aid);

                if (!(target->permissions & HKPermissionPairedRead)) {
                    json.setString("iid");
                    json.setInt(iid);
                    json.setString("status");
                    json.setInt(HAPStatusWriteOnly);
                    json.endObject();
                    continue;
                }

                target->serializeToJSON(json, nullptr, format, client);

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

    client->sendChunk(nullptr, 0);
}

void ESPHomeKit::onIdentify(HKClient *client) {
    HKLOGINFO("[HKClient::onIdentify] Identify\r\n");

    if (HKStorage::isPaired()) {
        client->sendJSONErrorResponse(400, HAPStatusInsufficientPrivileges);
        return;
    }
    
    char *message = (char *) malloc(sizeof(http_header_204));
    strcpy_P(message, http_header_204);
    client->send((uint8_t *) message, sizeof(http_header_204)-1);
    free(message);

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
        characteristicIdentify->setter(HKValue(HKFormatBool, true));
    }
}

void ESPHomeKit::onUpdateCharacteristics(HKClient *client, uint8_t *message, const size_t &messageSize) {
    HKLOGINFO("[HKClient::onUpdateCharacteristics] Update Characteristics\r\n");

    char *jsonBodyStr = (char *) malloc(messageSize+1);
    memcpy(jsonBodyStr, message, messageSize);
    jsonBodyStr[messageSize] = 0;
    String jsonBody = String(jsonBodyStr);
    free(jsonBodyStr);

    if (jsonBody.substring(0, 20) != "{\"characteristics\":[") {
        HKLOGERROR("[HKClient::onUpdateCharacteristics] Could not deserialize json beginning not equal\r\n");
        client->sendJSONErrorResponse(400, HAPStatusInvalidValue);
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

        int aidPos = item.indexOf("\"aid\"");
        int iidPos = item.indexOf("\"iid\"");
        int evPos = item.indexOf("\"ev\"");
        int valuePos = item.indexOf("\"value\"");
        if (aidPos == -1 || iidPos == -1 || (evPos == -1 && valuePos == -1)) {
            break;
        }

        int aidBeginPos = item.indexOf(':', aidPos);
        int aidEndPos = item.indexOf(',', aidPos);
        int iidBeginPos = item.indexOf(':', iidPos);
        int iidEndPos = item.indexOf(',', iidPos);
        if (aidBeginPos == -1 || aidEndPos == -1 || iidBeginPos == -1 || iidEndPos == -1) {
            break;
        }

        int aid = item.substring(aidBeginPos + 1, aidEndPos).toInt();
        int iid = item.substring(iidBeginPos + 1, iidEndPos).toInt();
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

        processUpdateCharacteristic(client, aid, iid, ev, value);

        cursor = end + 1;
    }

    char *response = (char *) malloc(sizeof(http_header_204));
    strcpy_P(response, http_header_204);
    client->send((uint8_t *) response, sizeof(http_header_204)-1);
    free(response);
}

HAPStatus ESPHomeKit::processUpdateCharacteristic(HKClient *client, uint aid, uint iid, String ev, String value) {
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
        status = characteristic->setEvent(client, ev);
    }
    return status;
}

/**
 * @brief Handle /pair-setup HomeKit endpoint
 * 
 * @param Client Client that sent request
 * @param Message Body of the request
 * @param MessageSize Size of message
 */
void ESPHomeKit::onPairSetup(HKClient *client, uint8_t *message, const size_t &messageSize) {
    std::vector<HKTLV *> tlvs = HKTLV::parseTLV(message, messageSize);

    int8_t value = -1;
    HKTLV *state = HKTLV::findTLV(tlvs, TLVTypeState);
    if (state) {
        value = state->getIntValue();
    }

    switch (value) {
    case 1: {
        HKLOGINFO("[HKClient::onPairSetup] Setup Step 1/3\r\n");
        if (HKStorage::isPaired()) {
            HKLOGWARNING("[HKClient::onPairSetup] Refuse to pair: Already paired\r\n");

            client->sendTLVError(2, TLVErrorUnavailable);
            break;
        }

        if (isPairing()) {
            HKLOGWARNING("[HKClient::onPairSetup] Refuse to pair: another pairing in process\r\n");

            client->sendTLVError(2, TLVErrorBusy);
            break;
        }
        client->setPairing(true);
        srp->start();

        for (auto it = tlvs.begin(); it != tlvs.end();) {
            delete *it;
            it = tlvs.erase(it);
        }
        tlvs.push_back(new HKTLV(TLVTypeState, 2, 1));
        tlvs.push_back(new HKTLV(TLVTypePublicKey, srp->getB(), 384));
        tlvs.push_back(new HKTLV(TLVTypeSalt, srp->getSalt(), 16));

        client->sendTLVResponse(tlvs);
    }
    break;
    case 3: {
        HKLOGINFO("[HKClient::onPairSetup] Setup Step 2/3\r\n");
        HKTLV* publicKey = HKTLV::findTLV(tlvs, TLVTypePublicKey);
        HKTLV* proof = HKTLV::findTLV(tlvs, TLVTypeProof);
        if (!publicKey || !proof) {
            HKLOGWARNING("[HKClient::onPairSetup] Could not find Public Key or Proof in Message\r\n");
            client->sendTLVError(4, TLVErrorAuthentication);
            client->setPairing(false);
            break;
        }

        srp->setA(publicKey->getValue(), publicKey->getSize(), nullptr);

        if (srp->checkM1(proof->getValue(), proof->getSize())) {
            for (auto it = tlvs.begin(); it != tlvs.end();) {
                delete *it;
                it = tlvs.erase(it);
            }
            tlvs.push_back(new HKTLV(TLVTypeState, 4, 1));
            tlvs.push_back(new HKTLV(TLVTypeProof, srp->getM2(), 64));

            client->sendTLVResponse(tlvs);
        } else {
            //return error
            HKLOGERROR("[HKClient::onPairSetup] SRP Error\r\n");
            client->sendTLVError(4, TLVErrorAuthentication);
            client->setPairing(false);
        }
        break;
    }
    case 5: {
        HKLOGINFO("[HKClient::onPairSetup] Setup Step 3/3\r\n");

        uint8_t sharedSecret[32];
        const char salt1[] = "Pair-Setup-Encrypt-Salt";
        const char info1[] = "Pair-Setup-Encrypt-Info\001";
        hkdf(sharedSecret, srp->getK(), 64, (uint8_t *) salt1, sizeof(salt1)-1, (uint8_t *) info1, sizeof(info1)-1);

        HKTLV *encryptedTLV = HKTLV::findTLV(tlvs, TLVTypeEncryptedData);
        if (!encryptedTLV) {
            HKLOGERROR("[HKClient::onPairSetup] Failed: Could not find Encrypted Data\r\n");
            client->sendTLVError(6, TLVErrorAuthentication);
            client->setPairing(false);
            break;
        }

        size_t decryptedDataSize = encryptedTLV->getSize() - 16;
        uint8_t *decryptedData = (uint8_t *) malloc(decryptedDataSize);

        if (!crypto_verifyAndDecrypt(sharedSecret, (uint8_t *) "PS-Msg05", encryptedTLV->getValue(), decryptedDataSize, decryptedData, encryptedTLV->getValue() + decryptedDataSize)) {
            HKLOGERROR("[HKClient::onPairSetup] Decryption failed: MAC not equal\r\n");
            free(decryptedData);
            client->sendTLVError(6, TLVErrorAuthentication);
            client->setPairing(false);
            break;
        }

        std::vector<HKTLV *> decryptedMessage = HKTLV::parseTLV(decryptedData, decryptedDataSize);
        free(decryptedData);
        HKTLV *deviceId = HKTLV::findTLV(decryptedMessage, TLVTypeIdentifier);
        if (!deviceId) {
            HKLOGERROR("[HKClient::onPairSetup] Decryption failed: Device ID not found in decrypted Message\r\n");
            for (auto msg : decryptedMessage) {
                delete msg;
            }
            client->sendTLVError(6, TLVErrorAuthentication);
            client->setPairing(false);
            break;
        }

        HKTLV *publicKey = HKTLV::findTLV(decryptedMessage, TLVTypePublicKey);
        if (!publicKey) {
            HKLOGERROR("[HKClient::onPairSetup] Decryption failed: Public Key not found in decrypted Message\r\n");
            for (auto msg : decryptedMessage) {
                delete msg;
            }
            client->sendTLVError(6, TLVErrorAuthentication);
            client->setPairing(false);
            break;
        }

        HKTLV *signature = HKTLV::findTLV(decryptedMessage, TLVTypeSignature);
        if (!signature) {
            HKLOGERROR("[HKClient::onPairSetup] Decryption failed: Signature not found in decrypted Message\r\n");
            for (auto msg : decryptedMessage) {
                delete msg;
            }
            client->sendTLVError(6, TLVErrorAuthentication);
            client->setPairing(false);
            break;
        }

        uint8_t deviceX[32];
        const char salt2[] = "Pair-Setup-Controller-Sign-Salt";
        const char info2[] = "Pair-Setup-Controller-Sign-Info\001";
        hkdf(deviceX, srp->getK(), 64, (uint8_t *) salt2, sizeof(salt2)-1, (uint8_t *) info2, sizeof(info2)-1);

        uint64_t deviceInfoSize = sizeof(deviceX) + deviceId->getSize() + publicKey->getSize();
        uint8_t *deviceInfo = (uint8_t *) malloc(deviceInfoSize);
        memcpy(deviceInfo, deviceX, sizeof(deviceX));
        memcpy(deviceInfo + sizeof(deviceX), deviceId->getValue(), deviceId->getSize());
        memcpy(deviceInfo + sizeof(deviceX) + deviceId->getSize(), publicKey->getValue(), publicKey->getSize());

        if (!Ed25519::verify(signature->getValue(), publicKey->getValue(), deviceInfo, deviceInfoSize)) {
            HKLOGERROR("[HKClient::onPairSetup] Could not verify Ed25519 Device Info, Signature and Public Key\r\n");
            free(deviceInfo);
            for (auto msg : decryptedMessage) {
                delete msg;
            }
            client->sendTLVError(6, TLVErrorAuthentication);
            client->setPairing(false);
            break;
        }
        free(deviceInfo);

        int result = HKStorage::addPairing((char *) deviceId->getValue(), publicKey->getValue(), 1);
        if (result) {
            HKLOGERROR("[HKClient::onPairSetup] COULD NOT STORE PAIRING\r\n");
        }

        // M6 Response Generation
        String accessoryId = HKStorage::getAccessoryId();
        size_t accessoryInfoSize = 32 + accessoryId.length() + 32;
        uint8_t *accessoryInfo = (uint8_t *) malloc(accessoryInfoSize);
        const char salt3[] = "Pair-Setup-Accessory-Sign-Salt";
        const char info3[] = "Pair-Setup-Accessory-Sign-Info\001";
        hkdf(accessoryInfo, srp->getK(), 64, (uint8_t *) salt3, sizeof(salt3)-1, (uint8_t *) info3, sizeof(info3)-1);
        delete srp;

        memcpy(accessoryInfo + 32, accessoryId.c_str(), accessoryId.length());
        memcpy(accessoryInfo + 32 + accessoryId.length(), HKStorage::getAccessoryKey().publicKey, 32);

        uint8_t accessorySignature[64];
        Ed25519::sign(accessorySignature, HKStorage::getAccessoryKey().privateKey, HKStorage::getAccessoryKey().publicKey, accessoryInfo, accessoryInfoSize);
        free(accessoryInfo);

        std::vector<HKTLV *> responseMessage = {
                new HKTLV(TLVTypeIdentifier, (uint8_t *) accessoryId.c_str(), accessoryId.length()),
                new HKTLV(TLVTypePublicKey, HKStorage::getAccessoryKey().publicKey, 32),
                new HKTLV(TLVTypeSignature, accessorySignature, 64)
        };
        size_t responseDataSize = HKTLV::getFormattedTLVSize(responseMessage);
        uint8_t *responseData = (uint8_t *) malloc(responseDataSize);
        HKTLV::formatTLV(responseMessage, responseData);

        uint8_t *encryptedResponseData = (uint8_t *) malloc(responseDataSize + 16);
        crypto_encryptAndSeal(sharedSecret, (uint8_t *) "PS-Msg06", responseData, responseDataSize, encryptedResponseData, encryptedResponseData + responseDataSize);
        free(responseData);

        for (auto msg : decryptedMessage) {
            delete msg;
        }
        for (auto it = tlvs.begin(); it != tlvs.end();) {
            delete *it;
            it = tlvs.erase(it);
        }
        tlvs.push_back(new HKTLV(TLVTypeState, 6, 1));
        tlvs.push_back(new HKTLV(TLVTypeEncryptedData, encryptedResponseData, responseDataSize + 16));
        free(encryptedResponseData);

        client->sendTLVResponse(tlvs);
        client->setPairing(false);

        setupMDNS();
        HKLOGINFO("[HKClient::onPairSetup] Pairing Successfull\r\n");
        break;
    }
    break;
    default:
        break;
    }

    for (auto it = tlvs.begin(); it != tlvs.end();) {
        delete *it;
        it = tlvs.erase(it);
    }
}

/**
 * @brief Handle /pair-verify HomeKit endpoint
 * 
 * @param Client Client that sent request
 * @param Message Body of the request
 * @param MessageSize Size of message
 */
void ESPHomeKit::onPairVerify(HKClient *client, uint8_t *message, const size_t &messageSize) {
    std::vector<HKTLV *> tlvs = HKTLV::parseTLV(message, messageSize);

    int8_t value = -1;
    HKTLV *state = HKTLV::findTLV(tlvs, TLVTypeState);
    if (state) {
        value = state->getIntValue();
    }

    switch (value) {
    case 1: {
        HKLOGINFO("[HKClient::onPairVerify] Verify Step 1/2\r\n");
        HKTLV *deviceKeyTLV = HKTLV::findTLV(tlvs, TLVTypePublicKey);
        if (!deviceKeyTLV) {
            HKLOGERROR("[HKClient::onPairVerify] Device Key not Found\r\n");
            client->sendTLVError(2, TLVErrorUnknown);
            break;
        }

        size_t encryptedResponseSize = client->prepareEncryption(nullptr, nullptr, nullptr);
        uint8_t accessoryPublicKey[32];
        uint8_t *encryptedResponseData = (uint8_t *) malloc(encryptedResponseSize);
        client->prepareEncryption(accessoryPublicKey, encryptedResponseData, deviceKeyTLV->getValue());

        std::vector<HKTLV *> responseMessage = {
                new HKTLV(TLVTypeState, 2, 1),
                new HKTLV(TLVTypePublicKey, accessoryPublicKey, 32),
                new HKTLV(TLVTypeEncryptedData, encryptedResponseData, encryptedResponseSize)
        };
        free(encryptedResponseData);

        client->sendTLVResponse(responseMessage);
    }
    break;
    case 3: {
        HKLOGINFO("[HKClient::onPairVerify] Verify Step 2/2\r\n");

        if (!client->didStartEncryption()) {
            client->sendTLVError(4, TLVErrorAuthentication);
            break;
        }

        HKTLV *encryptedData = HKTLV::findTLV(tlvs, TLVTypeEncryptedData);
        if (!encryptedData) {
            HKLOGERROR("[HKClient::onPairVerify] Could not find encrypted data\r\n");
            client->sendTLVError(4, TLVErrorUnknown);
            client->resetEncryption();
            break;
        }

        if (!client->finishEncryption(encryptedData->getValue(), encryptedData->getSize())) {
            HKLOGERROR("[HKClient::onPairVerify] Could not finish encryption\r\n");
            client->sendTLVError(4, TLVErrorAuthentication);
            break;
        }

        std::vector<HKTLV *> responseMessage = {
                new HKTLV(TLVTypeState, 4, 1)
        };
        client->sendTLVResponse(responseMessage);

        client->resetEncryption();
        client->setEncryption(true);
    }
    break;
    default:
        break;
    }

    for (auto msg : tlvs) {
        delete msg;
    }
}

void ESPHomeKit::onPairings(HKClient *client, uint8_t *message, const size_t &messageSize) {
    HKLOGINFO("[HKClient::onPairings] Pairings\r\n");

    std::vector<HKTLV *> tlvs = HKTLV::parseTLV(message, messageSize);
    HKTLV *state = HKTLV::findTLV(tlvs, TLVTypeState);
    if (!state || state->getIntValue() != 1) {
        client->sendTLVError(2, TLVErrorUnknown);

        for (auto msg : tlvs) {
            delete msg;
        }
        HKLOGERROR("[HKClient::onPairings] Unknown State\r\n");
        return;
    }

    HKTLV *method = HKTLV::findTLV(tlvs, TLVTypeMethod);
    if (!method) {
        client->sendTLVError(2, TLVErrorUnknown);

        for (auto msg : tlvs) {
            delete msg;
        }
        HKLOGERROR("[HKClient::onPairings] Unknown Message\r\n");
        return;
    }

    switch ((TLVMethod) method->getIntValue()) {
    case TLVMethodPairSetup:
        onPairSetup(client, message, messageSize);
        break;
    case TLVMethodPairVerify:
        onPairVerify(client, message, messageSize);
        break;
    case TLVMethodAddPairing: {
        HKLOGINFO("[HKClient::onPairings] Add Pairing\r\n");
        if (!(client->getPermission() & PairingPermissionAdmin)) {
            HKLOGWARNING("[HKClient::onPairings] Refusing to add pairing to non-admin controller\r\n");
            client->sendTLVError(2, TLVErrorAuthentication);
            break;
        }

        HKTLV *deviceId = HKTLV::findTLV(tlvs, TLVTypeIdentifier);
        if (!deviceId) {
            HKLOGWARNING("[HKClient::onPairings] Invalid add pairing request: no device identifier\r\n");
            client->sendTLVError(2, TLVErrorUnknown);
            break;
        }

        HKTLV *devicePublicKey = HKTLV::findTLV(tlvs, TLVTypePublicKey);
        if (!devicePublicKey) {
            HKLOGWARNING("[HKClient::onPairings] Invalid add pairing request: no device public key\r\n");
            client->sendTLVError(2, TLVErrorUnknown);
            break;
        }

        HKTLV *devicePermission = HKTLV::findTLV(tlvs, TLVTypePermissions);
        if (!devicePermission) {
            HKLOGWARNING("[HKClient::onPairings] Invalid add pairing request: no device Permissions\r\n");
            client->sendTLVError(2, TLVErrorUnknown);
            break;
        }

        char *deviceIdentifier = (char *) malloc(deviceId->getSize());
        strncpy(deviceIdentifier, (char *) deviceId->getValue(), deviceId->getSize());
        Pairing *comparePairing = HKStorage::findPairing(deviceIdentifier);
        if (comparePairing) {
            if (devicePublicKey->getSize() != 32 || memcmp(devicePublicKey->getValue(), comparePairing->deviceKey, 32) != 0) {
                HKLOGWARNING("[HKClient::onPairings] Failed to add pairing: pairing public key differs from given one\r\n");
                free(deviceIdentifier);
                delete comparePairing;
                client->sendTLVError(2, TLVErrorUnknown);
                break;
            }
            delete comparePairing;

            if (HKStorage::updatePairing(deviceIdentifier, *devicePermission->getValue())) {
                HKLOGWARNING("[HKClient::onPairings] Failed to add pairing: storage error\r\n");
                free(deviceIdentifier);
                client->sendTLVError(2, TLVErrorUnknown);
                break;
            }

            HKLOGINFO("[HKClient::onPairings] Updated pairing with id=%s\r\n", deviceIdentifier);
        } else {
            int r = HKStorage::addPairing(deviceIdentifier, devicePublicKey->getValue(), *devicePermission->getValue());
            if (r == -2) {
                HKLOGWARNING("[HKClient::onPairings] Failed to add pairing: max peers\r\n");
                free(deviceIdentifier);
                client->sendTLVError(2, TLVErrorMaxPeers);
                break;
            } else if (r != 0) {
                HKLOGWARNING("[HKClient::onPairings] Failed to add pairing: Storage error\r\n");
                free(deviceIdentifier);
                client->sendTLVError(2, TLVErrorUnknown);
                break;
            }

            delete comparePairing;

            HKLOGINFO("[HKClient::onPairings] Added pairing with id=%s\r\n", deviceIdentifier);
        }
        free(deviceIdentifier);

        std::vector<HKTLV *> response = {
            new HKTLV(TLVTypeState, 2, 1),
        };
        client->sendTLVResponse(response);
        for (auto responseItem : response) {
            delete responseItem;
        }
        break;
    }
    case TLVMethodRemovePairing: {
        HKLOGINFO("[HKClient::onPairings] Remove pairing\r\n");

        if (!(client->getPermission() & PairingPermissionAdmin)) {
            HKLOGWARNING("[HKClient::onPairings] Refuse to remove pairing to non-admin controller\r\n");
            client->sendTLVError(2, TLVErrorAuthentication);
            break;
        }

        HKTLV *deviceId = HKTLV::findTLV(tlvs, TLVTypeIdentifier);
        if (!deviceId) {
            HKLOGWARNING("[HKClient::onPairings] Invalid remove pairing request: no device identifier\r\n");
            client->sendTLVError(2, TLVErrorUnknown);
            break;
        }

        char *deviceIdentifier = (char *) malloc(deviceId->getSize());
        strncpy(deviceIdentifier, (char *) deviceId->getValue(), deviceId->getSize());
        Pairing *comparePairing = HKStorage::findPairing(deviceIdentifier);
        if (comparePairing) {
            bool isAdmin = comparePairing->permissions & PairingPermissionAdmin;

            int result = HKStorage::removePairing(deviceIdentifier);
            if (result) {
                free(deviceIdentifier);
                delete comparePairing;
                HKLOGERROR("[HKClient::onPairings] Failed to remove pairing: storage error\r\n");
                client->sendTLVError(2, TLVErrorUnknown);
                break;
            }

            HKLOGINFO("[HKClient::onPairings] Removed pairing with id=%s\r\n", deviceIdentifier);
            free(deviceIdentifier);

            #if HKLOGLEVEL <= 1
            for (auto pPairing : HKStorage::getPairings()) {
                HKLOGINFO("[HKStorage] Pairing id=%i deviceId=%36s permissions=%i\r\n", pPairing->id, pPairing->deviceId, pPairing->permissions);
            }
            #endif

            for (auto client : clients) {
                if (client->getPairingId() == comparePairing->id) {
                    client->stop();
                }
            }

            if (isAdmin) {
                if (!HKStorage::hasPairedAdmin()) {
                    HKLOGINFO("[HKClient::onPairings] Last admin pairing was removed, enabling pair setup\r\n");
                    ESP.restart();
                }
            }
        }

        std::vector<HKTLV *> response = {
                new HKTLV(TLVTypeState, 2, 1)
        };
        client->sendTLVResponse(response);
        for (auto responseItem : response) {
            delete responseItem;
        }
        break;
    }
    case TLVMethodListPairings: {
        if (!(client->getPermission() & PairingPermissionAdmin)) {
            HKLOGWARNING("[HKClient::onPairings] Refusing to list pairings to non-admin controller\r\n");
            client->sendTLVError(2, TLVErrorAuthentication);
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

        client->sendTLVResponse(response);
        for (auto pairingItem : pairings) {
            delete pairingItem;
        }
        for (auto responseItem : response) {
            delete responseItem;
        }
        break;
    }
    }

    for (auto msg : tlvs) {
        delete msg;
    }
}
