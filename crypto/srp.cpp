/*
 * srp.c
 *
 * SRP algorithm (see http://srp.stanford.edu)
 * Uses bits of the ARM mbed TSL library (https://tls.mbed.org).
 *  Notes: You can probably do better here since N and G are known at compile time.
 *         This library uses a massive amount of RAM during calculations.
 *         The SRP handshake takes 30+ seconds.
 *
 *  Created on: Jun 10, 2015
 *      Author: tim
 */

#include "srp.h"

#define E(V)  (((V) << 24) | ((V) >> 24) | (((V) >> 8) & 0x0000FF00) | (((V) << 8) & 0x00FF0000))
static const uint32_t srp_N[] PROGMEM =
        {
                E(0xFFFFFFFF), E(0xFFFFFFFF), E(0xC90FDAA2), E(0x2168C234), E(0xC4C6628B), E(0x80DC1CD1), E(0x29024E08),
                E(0x8A67CC74), E(0x020BBEA6), E(0x3B139B22), E(0x514A0879), E(0x8E3404DD), E(0xEF9519B3), E(0xCD3A431B),
                E(0x302B0A6D), E(0xF25F1437), E(0x4FE1356D), E(0x6D51C245), E(0xE485B576), E(0x625E7EC6), E(0xF44C42E9),
                E(0xA637ED6B), E(0x0BFF5CB6), E(0xF406B7ED), E(0xEE386BFB), E(0x5A899FA5), E(0xAE9F2411), E(0x7C4B1FE6),
                E(0x49286651), E(0xECE45B3D), E(0xC2007CB8), E(0xA163BF05), E(0x98DA4836), E(0x1C55D39A), E(0x69163FA8),
                E(0xFD24CF5F), E(0x83655D23), E(0xDCA3AD96), E(0x1C62F356), E(0x208552BB), E(0x9ED52907), E(0x7096966D),
                E(0x670C354E), E(0x4ABC9804), E(0xF1746C08), E(0xCA18217C), E(0x32905E46), E(0x2E36CE3B), E(0xE39E772C),
                E(0x180E8603), E(0x9B2783A2), E(0xEC07A28F), E(0xB5C55DF0), E(0x6F4C52C9), E(0xDE2BCBF6), E(0x95581718),
                E(0x3995497C), E(0xEA956AE5), E(0x15D22618), E(0x98FA0510), E(0x15728E5A), E(0x8AAAC42D), E(0xAD33170D),
                E(0x04507A33), E(0xA85521AB), E(0xDF1CBA64), E(0xECFB8504), E(0x58DBEF0A), E(0x8AEA7157), E(0x5D060C7D),
                E(0xB3970F85), E(0xA6E1E4C7), E(0xABF5AE8C), E(0xDB0933D7), E(0x1E8C94E0), E(0x4A25619D), E(0xCEE3D226),
                E(0x1AD2EE6B), E(0xF12FFA06), E(0xD98A0864), E(0xD8760273), E(0x3EC86A64), E(0x521F2B18), E(0x177B200C),
                E(0xBBE11757), E(0x7A615D6C), E(0x770988C0), E(0xBAD946E2), E(0x08E24FA0), E(0x74E5AB31), E(0x43DB5BFC),
                E(0xE0FD108E), E(0x4B82D120), E(0xA93AD2CA), E(0xFFFFFFFF), E(0xFFFFFFFF)
        };
#undef E

#define srp_N_sizeof 384
#define srp_G 5

// Pre-compute hashes where we can
static const uint8_t srp_N_G_hash[] PROGMEM =
        {
                0xA9, 0xC2, 0xE2, 0x55, 0x9B, 0xF0, 0xEB, 0xB5, 0x3F, 0x0C, 0xBB, 0xF6, 0x22, 0x82, 0x90, 0x6B,
                0xED, 0xE7, 0xF2, 0x18, 0x2F, 0x00, 0x67, 0x82, 0x11, 0xFB, 0xD5, 0xBD, 0xE5, 0xB2, 0x85, 0x03,
                0x3A, 0x49, 0x93, 0x50, 0x3B, 0x87, 0x39, 0x7F, 0x9B, 0xE5, 0xEC, 0x02, 0x08, 0x0F, 0xED, 0xBC,
                0x08, 0x35, 0x58, 0x7A, 0xD0, 0x39, 0x06, 0x08, 0x79, 0xB8, 0x62, 0x1E, 0x8C, 0x36, 0x59, 0xE0
        };
static const uint8_t srp_N_hash_srp_G_hash[] PROGMEM =
        {
                0xB3, 0xD6, 0x3E, 0xF6, 0xAA, 0xFB, 0x2E, 0x97, 0x96, 0xDA, 0xE0, 0x06, 0xFE, 0x60, 0xF2, 0x0E,
                0x53, 0x26, 0xFE, 0x1E, 0x1C, 0x52, 0xA5, 0x02, 0x1E, 0xB2, 0x17, 0x47, 0xCA, 0x33, 0xDF, 0xEA,
                0x8F, 0xF2, 0x03, 0xBD, 0x45, 0xA2, 0x54, 0x43, 0x95, 0xD7, 0x3D, 0x19, 0x89, 0x9C, 0x17, 0x4A,
                0x68, 0x3C, 0x9E, 0x78, 0x32, 0x96, 0xDD, 0x16, 0x93, 0xEB, 0xC7, 0x1C, 0xF5, 0xA5, 0x3D, 0xA3
        };

const char pinMessage[] PROGMEM = "Pair-Setup:";

static void MPI_ERROR_CHECK(int CODE)
{
    if (CODE != 0)
    {
        printf("Found Error (Code ");
        printf("%d", CODE);
        printf(")");
        //APP_ERROR_CHECK(NRF_ERROR_INTERNAL);
    }
}

Srp::Srp(const char *pincode)
{
    int err_code;

    // The MPI library uses a ridiculous amount of memory. We use the stack allocator
    // so we don't tie this memory up except when we absolutely need to.
    //uint8_t memory[11 * 1024];
    //mbedtls_memory_buffer_alloc_init(memory, sizeof(memory));

    // Generate salt
    os_get_random(srp_salt, sizeof(srp_salt));
    //random_create(srp_salt, sizeof(srp_salt));

    // Generate 'b' - a random value
    os_get_random(srp_b, 32);

    // Calculate 'x' = H(s | H(I | ":" | P))
    mbedtls_mpi x;
    mbedtls_mpi_init(&x);
    {
        uint8_t *message = (uint8_t *) malloc(sizeof(srp_salt) + 64);
        memcpy(message, srp_salt, sizeof(srp_salt));

        char pinMessageCopy[21];
        memcpy_P(pinMessageCopy, pinMessage, 12);
        strcat(pinMessageCopy, pincode);

        crypto_hash_sha512(message + sizeof(srp_salt), (uint8_t *)pinMessageCopy, sizeof(pinMessageCopy));
        crypto_hash_sha512(message, message, sizeof(srp_salt) + 64);
        err_code = mbedtls_mpi_read_binary(&x, message, 64);
        free(message);
        MPI_ERROR_CHECK(err_code);
    }

    // Calculate 'v' = g ^ x mod N
    mbedtls_mpi g;
    mbedtls_mpi_init(&g);
    err_code = mbedtls_mpi_lset(&g, srp_G);
    MPI_ERROR_CHECK(err_code);

    mbedtls_mpi n;
    mbedtls_mpi_init(&n);
    uint32_t *srp_N_buf = (uint32_t *)malloc(sizeof(uint32_t) * srp_N_sizeof);
    memcpy_P(srp_N_buf, srp_N, srp_N_sizeof);
    err_code = mbedtls_mpi_read_binary(&n, (uint8_t*)srp_N_buf, srp_N_sizeof);
    MPI_ERROR_CHECK(err_code);
    free(srp_N_buf);

    mbedtls_mpi tmp;
    mbedtls_mpi_init(&tmp);
    mbedtls_mpi v;
    mbedtls_mpi_init(&v);
    err_code = mbedtls_mpi_exp_mod(&v, &g, &x, &n, &tmp);
    MPI_ERROR_CHECK(err_code);

    // Calculate 'k'
    mbedtls_mpi k;
    mbedtls_mpi_init(&k);
    uint8_t *srp_N_G_hash_buf = (uint8_t *)malloc(sizeof(srp_N_G_hash));
    memcpy_P(srp_N_G_hash_buf, srp_N_G_hash, sizeof(srp_N_G_hash));
    err_code = mbedtls_mpi_read_binary(&k, srp_N_G_hash_buf, sizeof(srp_N_G_hash));
    MPI_ERROR_CHECK(err_code);
    free(srp_N_G_hash_buf);

    // Calculate 'B' = k*v + g^b % N
    mbedtls_mpi B;
    mbedtls_mpi_init(&B);
    err_code = mbedtls_mpi_mul_mpi(&B, &k, &v);
    MPI_ERROR_CHECK(err_code);

    err_code = mbedtls_mpi_write_binary(&v, srp_v, sizeof(srp_v));
    MPI_ERROR_CHECK(err_code);
    mbedtls_mpi_free(&v);

    mbedtls_mpi b;
    mbedtls_mpi_init(&b);
    err_code = mbedtls_mpi_read_binary(&b, srp_b, sizeof(srp_b));
    MPI_ERROR_CHECK(err_code);

    err_code = mbedtls_mpi_exp_mod(&x, &g, &b, &n, &tmp);
    MPI_ERROR_CHECK(err_code);
    err_code = mbedtls_mpi_add_abs(&B, &B, &x);
    MPI_ERROR_CHECK(err_code);
    err_code = mbedtls_mpi_mod_mpi(&B, &B, &n);
    MPI_ERROR_CHECK(err_code);
    err_code = mbedtls_mpi_write_binary(&B, srp_B, sizeof(srp_B));
    MPI_ERROR_CHECK(err_code);

    mbedtls_mpi_free(&b);
    mbedtls_mpi_free(&B);
    mbedtls_mpi_free(&k);
    mbedtls_mpi_free(&tmp);
    mbedtls_mpi_free(&n);
    mbedtls_mpi_free(&g);
    mbedtls_mpi_free(&x);

    //mbedtls_memory_buffer_alloc_free();
}

void Srp::start()
{
    srp_clientM1 = 0;
    srp_serverM1 = 0;
}

uint8_t Srp::setA(uint8_t* abuf, uint16_t length, moretime_t moretime)
{
    int err_code;

    // The MPI library uses a ridiculous amount of memory. We use the stack allocator
    // so we don't tie this memory up except when we absolutely need to.
    //uint8_t memory[11 * 1024];
    //mbedtls_memory_buffer_alloc_init(memory, sizeof(memory));

    // getK
    {
        mbedtls_mpi s;
        mbedtls_mpi_init(&s);

        mbedtls_mpi n;
        mbedtls_mpi_init(&n);
        uint32_t *srp_N_buf = (uint32_t *)malloc(sizeof(uint32_t) * srp_N_sizeof);
        memcpy_P(srp_N_buf, srp_N, srp_N_sizeof);
        err_code = mbedtls_mpi_read_binary(&n, (uint8_t*)srp_N_buf, srp_N_sizeof);
        MPI_ERROR_CHECK(err_code);
        free(srp_N_buf);

        {
            // u = H(A | B)
            mbedtls_mpi u;
            mbedtls_mpi_init(&u);
            {
                uint8_t *message = (uint8_t *) malloc(length + sizeof(srp_B));
                memcpy(message, abuf, length);
                memcpy(message + length, srp_B, sizeof(srp_B));
                crypto_hash_sha512(message, message, length + sizeof(srp_B));

                err_code = mbedtls_mpi_read_binary(&u, message, 64);
                free(message);
                MPI_ERROR_CHECK(err_code);
            }

            mbedtls_mpi v;
            mbedtls_mpi_init(&v);
            err_code = mbedtls_mpi_read_binary(&v, srp_v, sizeof(srp_v));
            MPI_ERROR_CHECK(err_code);

            // getS = (A * v^u mod N)^b mod N
            err_code = mbedtls_mpi_exp_mod(&s, &v, &u, &n, NULL);
            MPI_ERROR_CHECK(err_code);

            mbedtls_mpi_free(&v);
            mbedtls_mpi_free(&u);
        }

        // These calculations take a long time. To avoid the connection dying we ask for more time now.
        if (moretime)
        {
            moretime();
        }

        {
            mbedtls_mpi a;
            mbedtls_mpi_init(&a);
            err_code = mbedtls_mpi_read_binary(&a, abuf, length);
            MPI_ERROR_CHECK(err_code);

            err_code = mbedtls_mpi_mul_mpi(&s, &s, &a);
            MPI_ERROR_CHECK(err_code);
            mbedtls_mpi_free(&a);

            mbedtls_mpi b;
            mbedtls_mpi_init(&b);
            err_code = mbedtls_mpi_read_binary(&b, srp_b, sizeof(srp_b));
            MPI_ERROR_CHECK(err_code);

            err_code = mbedtls_mpi_exp_mod(&s, &s, &b, &n, NULL);
            MPI_ERROR_CHECK(err_code);

            mbedtls_mpi_free(&b);
        }

        mbedtls_mpi_free(&n);

        uint8_t sbuf[384];
        err_code = mbedtls_mpi_write_binary(&s, sbuf, sizeof(sbuf));
        MPI_ERROR_CHECK(err_code);

        mbedtls_mpi_free(&s);

        crypto_hash_sha512(srp_K, sbuf, sizeof(sbuf));
    }

    //mbedtls_memory_buffer_alloc_free();

    // getM1 - username s abuf srp_B K
    {
        size_t messageSize = sizeof(srp_N_hash_srp_G_hash) + 64 + sizeof(srp_salt) + length + 384 + sizeof(srp_K);
        uint8_t *message = (uint8_t *) malloc(messageSize);
        uint8_t *srp_N_hash_srp_G_hash_buf = (uint8_t *)malloc(sizeof(srp_N_hash_srp_G_hash));
        memcpy_P(srp_N_hash_srp_G_hash_buf, srp_N_hash_srp_G_hash, sizeof(srp_N_hash_srp_G_hash));
        memcpy(message, srp_N_hash_srp_G_hash, sizeof(srp_N_hash_srp_G_hash));
        free(srp_N_hash_srp_G_hash_buf);

        char pinMessageCopy[10];
        memcpy_P(pinMessageCopy, pinMessage, 10);

        crypto_hash_sha512(message + sizeof(srp_N_hash_srp_G_hash), (uint8_t *) pinMessageCopy, 10); // First 10 chars only - not the PIN part
        memcpy(message + sizeof(srp_N_hash_srp_G_hash) + 64, srp_salt, sizeof(srp_salt));
        memcpy(message + sizeof(srp_N_hash_srp_G_hash) + 64 + sizeof(srp_salt), abuf, length);
        memcpy(message + sizeof(srp_N_hash_srp_G_hash) + 64 + sizeof(srp_salt) + length, srp_B, sizeof(srp_B));
        memcpy(message + sizeof(srp_N_hash_srp_G_hash) + 64 + sizeof(srp_salt) + length + 384, srp_K, sizeof(srp_K));
        srp_serverM1 = 1;
        if (srp_clientM1)
        {
            uint8_t hash[64];
            crypto_hash_sha512(hash, message, messageSize);
            if (memcmp(hash, srp_M1, sizeof(srp_M1)) != 0)
            {
                free(message);
                return 0;
            }
        }
        else
        {
            crypto_hash_sha512(srp_M1, message, messageSize);
        }
        free(message);
    }

    // getM2
    {
        uint8_t *message = (uint8_t *) malloc(length + sizeof(srp_M1) + sizeof(srp_K));
        memcpy(message, abuf, length);
        memcpy(message + length, srp_M1, sizeof(srp_M1));
        memcpy(message + length + sizeof(srp_M1), srp_K, sizeof(srp_K));
        crypto_hash_sha512(srp_M2, message, length + sizeof(srp_M1) + sizeof(srp_K));
        free(message);
    }

    return 1;
}

uint8_t* Srp::getSalt(void)
{
    return srp_salt;
}

uint8_t* Srp::getB(void)
{
    return srp_B;
}

uint8_t Srp::checkM1(uint8_t* m1, uint16_t length)
{
    srp_clientM1 = 1;
    if (length != sizeof(srp_M1))
    {
        return 0;
    }
    if (srp_serverM1)
    {
        if (memcmp(srp_M1, m1, sizeof(srp_M1)) != 0)
        {
            return 0;
        }
    }
    else
    {
        memcpy(srp_M1, m1, sizeof(srp_M1));
    }
    return 1;
}

uint8_t* Srp::getM2(void)
{
    return srp_M2;
}

uint8_t* Srp::getK(void)
{
    return srp_K;
}

void crypto_sha512hmac(uint8_t* hash, uint8_t* salt, uint8_t salt_length, uint8_t* data, uint8_t data_length)
{
    uint8_t *message1 = (uint8_t *) malloc(128 + data_length);
    uint8_t message2[128 + 64];

    memset(message1, 0x36, 128);
    memset(message2, 0x5C, 128);
    for (unsigned i = salt_length; i--; )
    {
        message1[i] = 0x36 ^ salt[i];
        message2[i] = 0x5C ^ salt[i];
    }
    memcpy(message1 + 128, data, data_length);
    crypto_hash_sha512(message2 + 128, message1, 128 + data_length);
    free(message1);
    crypto_hash_sha512(hash, message2, sizeof(message2));
}

void crypto_hkdf(uint8_t* target, uint8_t* salt, uint8_t salt_length, uint8_t* info, uint8_t info_length, uint8_t* ikm, uint8_t ikm_length)
{
    crypto_sha512hmac(target, salt, salt_length, ikm, ikm_length);
    crypto_sha512hmac(target, target, 64, info, info_length);
}

uint8_t crypto_verifyAndDecrypt(const uint8_t* key, uint8_t* nonce, uint8_t* encrypted, uint8_t length, uint8_t* output_buf, uint8_t* mac)
{
    uint8_t zeros64[64];
    memset(zeros64, 0, 64);
    uint8_t polykey[64];
    crypto_stream_chacha20_xor(polykey, zeros64, sizeof(zeros64), nonce, key, 0);

    uint8_t padding = (16 - length % 16) % 16;
    uint8_t *message = (uint8_t *) malloc(length + padding + 16);
    memcpy(message, encrypted, length);
    memset(message + length, 0, padding + 16);
    message[length + padding + 8] = (uint8_t)length;
    message[length + padding + 9] = (uint8_t)(length >> 8);

    if (crypto_onetimeauth_poly1305_verify(mac, message, length + padding + 16, polykey) != 0)
    {
        // Fail
        free(message);
        return 0;
    }
    else
    {
        crypto_stream_chacha20_xor(output_buf, message, length, nonce, key, 1);
        free(message);
        return 1;
    }
}

void crypto_encryptAndSeal(const uint8_t* key, uint8_t* nonce, uint8_t* plain, uint16_t length, uint8_t* output_buf, uint8_t* output_mac)
{
    uint8_t zeros64[64];
    memset(zeros64, 0, 64);
    uint8_t polykey[64];
    crypto_stream_chacha20_xor(polykey, zeros64, sizeof(zeros64), nonce, key, 0);

    uint8_t padding = (16 - length % 16) % 16;
    uint8_t *message = (uint8_t *) malloc(length + padding + 16);

    crypto_stream_chacha20_xor(message, plain, length, nonce, key, 1);
    memset(message + length, 0, padding + 16);
    message[length + padding + 8] = (uint8_t)length;
    message[length + padding + 9] = (uint8_t)(length >> 8);

    crypto_onetimeauth_poly1305(output_mac, message, length + padding + 16, polykey);

    memcpy(output_buf, message, length);
    free(message);
}

void hkdf(uint8_t *target, uint8_t *ikm, uint8_t ikmLength, uint8_t *salt, uint8_t saltLength, uint8_t *info, uint8_t infoLength) {
    uint8_t prk[64];
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
