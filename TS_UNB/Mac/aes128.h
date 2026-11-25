#ifndef AES128_H
#define AES128_H

#include <stdint.h>

#define AES128_BLOCK_SIZE  16
#define AES128_KEY_SIZE    16

/**
 * @brief  Encrypt one 16-byte block with AES-128 (FIPS-197).
 * @param  key  16-byte key
 * @param  in   16-byte plaintext block
 * @param  out  16-byte ciphertext block (may be the same buffer as in)
 */
void aes128_encrypt(const uint8_t key[AES128_KEY_SIZE],
                    const uint8_t in[AES128_BLOCK_SIZE],
                    uint8_t out[AES128_BLOCK_SIZE]);

#endif /* AES128_H */
