//
// Created by Max Vissing on 2019-05-04.
//

#include "HKTLV.h"

HKTLV::HKTLV(byte type, byte *value, size_t size) : type(type), size(size) {
    this->value = (byte *) malloc(size);
    memcpy(this->value, value, size);
}

HKTLV::HKTLV(byte type, uint8_t pValue, size_t size) : type(type), size(size) {
    byte *data = (byte *) malloc(size);
    for (size_t i = 0; i < size; i++) {
        data[i] = pValue & 0xff;
        pValue >>= 8;
    }

    value = data;
}

HKTLV::~HKTLV() {
    free(value);
}

byte HKTLV::getType() const {
    return type;
}

size_t HKTLV::getSize() const {
    return size;
}

byte *HKTLV::getValue() {
    return value;
}

std::vector<byte> HKTLV::formatTLV(const std::vector<HKTLV *> &values) {
    std::vector<byte> buffer;
    for (auto tlv : values) {
        if (!tlv->getSize()) {
            buffer.push_back(tlv->getType());
            buffer.push_back(0);
            continue;
        }

        int j = 0;
        int remainingSize = tlv->getSize();
        while (remainingSize) {
            buffer.push_back(tlv->getType());
            size_t chunkSize = (remainingSize > 255) ? 255 : remainingSize;
            buffer.push_back(chunkSize);
            for (size_t i = 0; i < chunkSize; i++, j++) {
                buffer.push_back(tlv->getValue()[j]);
            }
            remainingSize -= chunkSize;
        }
    }
    return buffer;
}

std::vector<HKTLV *> HKTLV::parseTLV(const std::vector<byte> &body) {
    std::vector<HKTLV *> message;

    size_t i = 0;
    while (i < body.size()) {
        byte type = body[i];
        size_t size = 0;
        std::vector<byte> data;

        size_t j = i;
        while (j < body.size() && body[j] == type && (byte)body[j+1] == 255) {
            size_t chunkSize = body[j+1];
            size += chunkSize;
            j += chunkSize + 2;
        }
        if (j < body.size() && body[j] == type) {
            size_t chunkSize = body[j+1];
            size += chunkSize;
        }

        if (size != 0) {
            size_t remaining = size;
            while (remaining) {
                size_t chunkSize = body[i+1];
                for (size_t k = 0; k < chunkSize; k++) {
                    data.push_back(body[i+2+k]);
                }
                i += chunkSize + 2;
                remaining -= chunkSize;
            }
        }

        message.push_back(new HKTLV(type, data.data(), data.size()));
    }
    return message;
}

std::vector<HKTLV *> HKTLV::parseTLV(const byte *body, size_t size) {
    std::vector<HKTLV *> message;
    for (size_t i = 0; i < size;) {
        byte type = body[i];
        size_t tlvSize = 0;
        std::vector<byte> data;

        size_t j = i;
        for (; j < size && body[j] == type && (byte)body[j+1] == 255; j += body[j+1] + 2) {
            tlvSize += body[j+1];
        }
        if (j < size && body[j] == type) {
            tlvSize += body[j+1];
        }

        if (size != 0) {
            while (tlvSize) {
                size_t chunkSize = body[i+1];
                for (size_t k = 0; k < chunkSize; k++) {
                    data.push_back(body[i+2+k]);
                }
                i+= chunkSize + 2;
                tlvSize -= chunkSize;
            }
        }

        message.push_back(new HKTLV(type, data.data(), data.size()));
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
