/*
 * srp.h
 *
 *  Created on: Jun 10, 2015
 *      Author: tim
 */
//
// Created by Max Vissing on 2019-05-24.
//

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HAP_SERVER_SRP_H
#define HAP_SERVER_SRP_H

#define BIGNUM_BYTES        384
#define BIGNUM_WORDS        (BIGNUM_BYTES / 4)

#include <mbedtls/bignum.h>
#include <mbedtls/sha512.h>
#include "tweetnacl.h"

typedef struct
{
    uint8_t b[32];
    uint8_t salt[16];
    uint8_t v[384];
    uint8_t B[384];

    uint8_t K[64];
    uint8_t M1[64];
    uint8_t M2[64];

    uint8_t clientM1:1;
    uint8_t serverM1:1;
} srp_keys_t;

extern srp_keys_t srp;

typedef void (*moretime_t)(void);

extern void srp_init(const char *pinMessage);
extern void srp_start(void);
extern uint8_t srp_setA(uint8_t* a, uint16_t length, moretime_t moretime);
extern uint8_t srp_checkM1(uint8_t* m1, uint16_t length);
extern uint8_t* srp_getSalt(void);
extern uint8_t* srp_getB(void);
extern uint8_t* srp_getM2(void);
extern uint8_t* srp_getK(void);

extern char *srp_pinMessage(void);

extern void crypto_sha512hmac(uint8_t* hash, uint8_t* salt, uint8_t salt_length, uint8_t* data, uint8_t data_length);

extern uint8_t crypto_verifyAndDecryptAAD(const uint8_t* key, uint8_t* nonce, uint8_t *aad, uint8_t aadLength, uint8_t* encrypted, uint8_t length, uint8_t* output_buf, uint8_t* mac);
extern uint8_t crypto_verifyAndDecrypt(const uint8_t* key, uint8_t* nonce, uint8_t* encrypted, uint8_t length, uint8_t* output_buf, uint8_t* mac);
extern void crypto_encryptAndSealAAD(const uint8_t* key, uint8_t* nonce, uint8_t *aad, uint8_t aadLength, uint8_t* plain, uint16_t length, uint8_t* output_buf, uint8_t* output_mac);
extern void crypto_encryptAndSeal(const uint8_t* key, uint8_t* nonce, uint8_t* plain, uint16_t length, uint8_t* output_buf, uint8_t* output_mac);

#endif //HAP_SERVER_SRP_H

#ifdef __cplusplus
}
#endif
