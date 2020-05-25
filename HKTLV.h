//
// Created by Max Vissing on 2019-05-04.
//

#ifndef HAP_SERVER_HKTLV_H
#define HAP_SERVER_HKTLV_H

#include <Arduino.h>

enum TLVType {
    TLVTypeMethod = 0,        // (integer) Method to use for pairing. See PairMethod
    TLVTypeIdentifier = 1,    // (UTF-8) Identifier for authentication
    TLVTypeSalt = 2,          // (bytes) 16+ bytes of random salt
    TLVTypePublicKey = 3,     // (bytes) Curve25519, SRP public key or signed Ed25519 key
    TLVTypeProof = 4,         // (bytes) Ed25519 or SRP proof
    TLVTypeEncryptedData = 5, // (bytes) Encrypted data with auth tag at end
    TLVTypeState = 6,         // (integer) State of the pairing process. 1=M1, 2=M2, etc.
    TLVTypeError = 7,         // (integer) Error code. Must only be present if error code is not 0. See TLVError
    TLVTypeRetryDelay = 8,    // (integer) Seconds to delay until retrying a setup code
    TLVTypeCertificate = 9,   // (bytes) X.509 Certificate
    TLVTypeSignature = 10,    // (bytes) Ed25519
    TLVTypePermissions = 11,  // (integer) Bit value describing permissions of the controller being added. // None (0x00): Regular user // Bit 1 (0x01): Admin that is able to add and remove // pairings against the accessory
    TLVTypeFragmentData = 13, // (bytes) Non-last fragment of data. If length is 0, it's an ACK.
    TLVTypeFragmentLast = 14, // (bytes) Last fragment of data
    TLVTypeSeparator = 0xff,
};

enum TLVMethod {
    TLVMethodPairSetup = 1,
    TLVMethodPairVerify = 2,
    TLVMethodAddPairing = 3,
    TLVMethodRemovePairing = 4,
    TLVMethodListPairings = 5,
};

enum TLVError {
    TLVErrorUnknown = 1,
    TLVErrorAuthentication = 2,
    TLVErrorBackoff = 3,
    TLVErrorMaxPeers = 4,
    TLVErrorMaxTries = 5,
    TLVErrorUnavailable = 6,
    TLVErrorBusy = 7
};

class HKTLV {
public:
    HKTLV(byte type, byte *value, size_t size);
    HKTLV(byte type, uint8_t pValue, size_t size);
    ~HKTLV();
    byte getType() const;
    size_t getSize() const;
    byte *getValue();
    int getIntValue();
    static std::vector<byte> formatTLV(const std::vector<HKTLV *> &values);
    static std::vector<HKTLV *> parseTLV(const std::vector<byte> &body);
    static std::vector<HKTLV *> parseTLV(const byte *body, size_t size);
    static HKTLV *findTLV(const std::vector<HKTLV *> &values, const TLVType &type);
private:
    byte type;
    byte *value;
    size_t size;
};


#endif //HAP_SERVER_HKTLV_H
