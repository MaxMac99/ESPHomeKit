/*
 * srp.h
 *
 *  Created on: Jun 10, 2015
 *      Author: tim
 */
//
// Created by Max Vissing on 2019-05-24.
//

#ifndef HAP_SERVER_SRP_H
#define HAP_SERVER_SRP_H

#define BIGNUM_BYTES        384
#define BIGNUM_WORDS        (BIGNUM_BYTES / 4)

#include <mbedtls/bignum.h>
#include <mbedtls/sha512.h>
#include "tweetnacl.h"


typedef void (*moretime_t)(void);

class Srp {
public:
    Srp(const char *pinMessage);
    ~Srp() = default;
    void start();
    uint8_t setA(uint8_t *a, uint16_t length, moretime_t moretime);
    uint8_t checkM1(uint8_t *m1, uint16_t length);
    uint8_t *getSalt();
    uint8_t *getB();
    uint8_t *getM2();
    uint8_t *getK();
private:
    uint8_t srp_b[32];
    uint8_t srp_salt[16];
    uint8_t srp_v[384];
    uint8_t srp_B[384];

    uint8_t srp_K[64];
    uint8_t srp_M1[64];
    uint8_t srp_M2[64];

    uint8_t srp_clientM1:1;
    uint8_t srp_serverM1:1;
};

extern void crypto_sha512hmac(uint8_t* hash, uint8_t* salt, uint8_t salt_length, uint8_t* data, uint8_t data_length);

extern uint8_t crypto_verifyAndDecryptAAD(const uint8_t* key, uint8_t* nonce, uint8_t *aad, uint8_t aadLength, uint8_t* encrypted, uint8_t length, uint8_t* output_buf, uint8_t* mac);
extern uint8_t crypto_verifyAndDecrypt(const uint8_t* key, uint8_t* nonce, uint8_t* encrypted, uint8_t length, uint8_t* output_buf, uint8_t* mac);
extern void crypto_encryptAndSealAAD(const uint8_t* key, uint8_t* nonce, uint8_t *aad, uint8_t aadLength, uint8_t* plain, uint16_t length, uint8_t* output_buf, uint8_t* output_mac);
extern void crypto_encryptAndSeal(const uint8_t* key, uint8_t* nonce, uint8_t* plain, uint16_t length, uint8_t* output_buf, uint8_t* output_mac);

#endif //HAP_SERVER_SRP_H

