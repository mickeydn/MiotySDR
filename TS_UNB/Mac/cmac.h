#ifndef CMAC_H
#define CMAC_H

#include <stdint.h>

#define CMAC_SIZE  16

/**
 * @brief  AES-128-CMAC (NIST SP 800-38B / RFC 4493).
 * @param  key  16-byte key
 * @param  msg  message (may be NULL when len is 0)
 * @param  len  message length in bytes
 * @param  mac  16-byte output (mioty SIGN = the first 4 bytes)
 */
void aes_cmac(const uint8_t key[16], const uint8_t *msg, uint16_t len,
              uint8_t mac[CMAC_SIZE]);

#endif /* CMAC_H */
