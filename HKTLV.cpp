//
// Created by Max Vissing on 2019-05-04.
//

#include "HKTLV.h"

HKTLV::HKTLV(uint8_t type, uint8_t *value, size_t size) : type(type), size(size) {
    this->value = (uint8_t *) malloc(size);
    memcpy(this->value, value, size);
}

HKTLV::HKTLV(uint8_t type, uint8_t pValue, size_t size) : type(type), size(size) {
    uint8_t *data = (uint8_t *) malloc(size);
    for (size_t i = 0; i < size; i++) {
        data[i] = pValue & 0xff;
        pValue >>= 8;
    }

    value = data;
}

HKTLV::~HKTLV() {
    free(value);
}

uint8_t HKTLV::getType() const {
    return type;
}

size_t HKTLV::getSize() const {
    return size;
}

uint8_t *HKTLV::getValue() {
    return value;
}

size_t HKTLV::getFormattedTLVSize(const std::vector<HKTLV *> &values) {
    size_t messageSize = 0;
    for (HKTLV *tlv : values) {
        messageSize += 2;
        if (tlv->getSize() > 0) {
            messageSize += ((size_t) (tlv->getSize()-1)/255) * 2 + tlv->getSize();
        }
    }
    return messageSize;
}

void HKTLV::formatTLV(const std::vector<HKTLV *> &values, uint8_t *message) {
    size_t messagePos = 0;
    for (auto tlv : values) {
        if (tlv->getSize() == 0) {
            message[messagePos++] = tlv->getType();
            message[messagePos++] = 0;
        } else {
            size_t remainingSize = tlv->getSize();
            while (remainingSize > 0) {
                message[messagePos++] = tlv->getType();
                size_t chunkSize = (remainingSize > 255) ? 255 : remainingSize;
                message[messagePos++] = chunkSize;
                memcpy(message + messagePos, tlv->getValue() + (tlv->getSize() - remainingSize), chunkSize);
                messagePos += chunkSize;
                remainingSize -= chunkSize;
            }
        }
    }
}

std::vector<HKTLV *> HKTLV::parseTLV(const uint8_t *body, const size_t &size) {
    std::vector<HKTLV *> message;
    for (size_t i = 0; i < size;) {
        uint8_t type = body[i];
        size_t tlvSize = 0;

        size_t j = i+1;
        for (; j < size && body[j-1] == type; j += body[j] + 2) {
            tlvSize += body[j];
        }

        uint8_t data[tlvSize];
        size_t currentPos = 0;
        while (i < size && currentPos < tlvSize && body[i] == type) {
            uint8_t chunkSize = body[++i];
            memcpy(data + currentPos, body + ++i, chunkSize);
            i += chunkSize;
            currentPos += chunkSize;
        }

        message.push_back(new HKTLV(type, data, tlvSize));
    }
    return message;
}

HKTLV *HKTLV::findTLV(const std::vector<HKTLV *> &values, const TLVType &type) {
    auto findEncryptedData = std::find_if(values.begin(), values.end(), [type](const HKTLV *cmp) {
        return cmp->getType() == type;
    });
    if (findEncryptedData == values.end()) {
        return nullptr;
    }
    return *findEncryptedData;
}

int HKTLV::getIntValue() {
    int result = 0;
    for (int i = size-1; i >= 0; i--) {
        result = (result << 8) + value[i];
    }
    return result;
}
