#include "cmac.h"
#include "aes128.h"
#include <string.h>

// shift a 16-byte block one bit to the left; if the top bit falls out, XOR the last byte with 0x87
static void cmac_double(const uint8_t in[16], uint8_t out[16]){
    uint8_t carry = in[0] & 0x80;
    for (int i = 0; i < 15; i++){
        out[i] = (uint8_t)((in[i] << 1) | (in[i+1] >> 7));
    }
    out[15] = (uint8_t)(in[15] << 1);
    if (carry){
        out[15] ^= 0x87;
    }
}

// subkeys K1 and K2 (RFC 4493 section 2.3): L = AES(key, 0), K1 = 2*L, K2 = 2*K1
static void cmac_subkeys(const uint8_t key[16], uint8_t k1[16], uint8_t k2[16]){
    uint8_t L[16] = {0};
    aes128_encrypt(key, L, L);
    cmac_double(L, k1);
    cmac_double(k1, k2);
}

void aes_cmac(const uint8_t key[16], const uint8_t *msg, uint16_t len, uint8_t mac[16]){
    uint8_t k1[16], k2[16];
    uint8_t x[16] = {0};          // running CBC value
    uint8_t last[16];
    cmac_subkeys(key, k1, k2);

    // number of blocks; an empty message counts as one (padded) block
    uint16_t n = (len + 15) / 16;
    uint8_t complete = (n > 0) && (len % 16 == 0);
    if (n == 0){
        n = 1;
    }

    // last block: complete -> XOR with K1, otherwise pad with 0x80 00.. and XOR with K2
    const uint8_t *m_last = msg + 16 * (n - 1);
    if (complete){
        for (int i = 0; i < 16; i++){
            last[i] = m_last[i] ^ k1[i];
        }
    } else {
        uint8_t rem = (uint8_t)(len % 16);
        memset(last, 0, 16);
        if (rem > 0){
            memcpy(last, m_last, rem);
        }
        last[rem] = 0x80;
        for (int i = 0; i < 16; i++){
            last[i] ^= k2[i];
        }
    }

    // CBC chain over all blocks except the last
    for (uint16_t b = 0; b < n - 1; b++){
        for (int i = 0; i < 16; i++){
            x[i] ^= msg[16 * b + i];
        }
        aes128_encrypt(key, x, x);
    }

    // final block
    for (int i = 0; i < 16; i++){
        x[i] ^= last[i];
    }
    aes128_encrypt(key, x, mac);
}
