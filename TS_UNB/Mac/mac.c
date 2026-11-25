#include "mac.h"
#include "aes128.h"
#include "cmac.h"
#include <string.h>

#define MAC_IV_FLAGS_UL   0x00        // encryption flags for uplink (Table 4-67)
#define MAC_CIV_BLOCK     0xFFFF      // block counter used in the CMAC IV

/**
 * @brief  Build EIV/CIV (ETSI p.58-59):
 *         EUI64(8) | 0x00 | flags | packet counter(4, big endian) | block counter(2, big endian)
 */
static void mac_build_iv(const mac_ctx_t *ctx, uint8_t flags, uint32_t counter,
                         uint16_t block, uint8_t iv[16]){
    memcpy(iv, ctx->eui64, 8);
    iv[8]  = 0x00;
    iv[9]  = flags;
    iv[10] = (uint8_t)(counter >> 24);
    iv[11] = (uint8_t)(counter >> 16);
    iv[12] = (uint8_t)(counter >> 8);
    iv[13] = (uint8_t)(counter);
    iv[14] = (uint8_t)(block >> 8);
    iv[15] = (uint8_t)(block);
}

/**
 * @brief  AES-128 counter mode: data XOR AES(key, EIV), block counter 0, 1, ...
 */
static void mac_encrypt(const mac_ctx_t *ctx, uint32_t counter, uint8_t *data, uint8_t len){
    uint8_t iv[16];
    uint8_t ks[16];
    for (uint16_t block = 0; block * 16 < len; block++){
        mac_build_iv(ctx, MAC_IV_FLAGS_UL, counter, block, iv);
        aes128_encrypt(ctx->nwk_key, iv, ks);
        for (uint8_t i = 0; i < 16 && block * 16 + i < len; i++){
            data[block * 16 + i] ^= ks[i];
        }
    }
}

/**
 * @brief  SIGN = 4 MSBytes of AES-CMAC(key, CIV | MPDU without SIGN)
 */
static void mac_sign(const mac_ctx_t *ctx, uint32_t counter, const uint8_t *mpdu,
                     uint8_t len, uint8_t sign[MAC_SIGN_SIZE]){
    uint8_t msg[16 + MAC_MAX_MPDU_LEN];
    uint8_t mac[CMAC_SIZE];
    mac_build_iv(ctx, MAC_IV_FLAGS_UL, counter, MAC_CIV_BLOCK, msg);
    memcpy(&msg[16], mpdu, len);
    aes_cmac(ctx->nwk_key, msg, (uint16_t)(16 + len), mac);
    memcpy(sign, mac, MAC_SIGN_SIZE);
}

uint8_t mac_build_uplink(mac_ctx_t *ctx, const uint8_t *payload, uint8_t len,
                         uint8_t flags, uint8_t *mpdu_out){
    uint8_t addr_len = ctx->use_short_addr ? 2 : 8;
    uint8_t overhead = 1 + addr_len + MAC_COUNTER_SIZE + MAC_SIGN_SIZE;
    if (len == 0 || overhead + len > MAC_MAX_MPDU_LEN){
        return 0;
    }

    uint32_t counter = ctx->packet_counter;
    uint8_t pos = 0;

    // MAC header: only the response / RX-open bits come from the caller
    uint8_t header = flags & (MAC_HDR_RESPONSE | MAC_HDR_RX_OPEN);
    if (!ctx->use_short_addr){
        header |= MAC_HDR_ADDR_EUI64;
    }
    mpdu_out[pos++] = header;

    // address
    if (ctx->use_short_addr){
        mpdu_out[pos++] = (uint8_t)(ctx->short_addr >> 8);
        mpdu_out[pos++] = (uint8_t)(ctx->short_addr);
    } else {
        memcpy(&mpdu_out[pos], ctx->eui64, 8);
        pos += 8;
    }

    // MPDUCNT: 24 LSB of the packet counter, big endian
    mpdu_out[pos++] = (uint8_t)(counter >> 16);
    mpdu_out[pos++] = (uint8_t)(counter >> 8);
    mpdu_out[pos++] = (uint8_t)(counter);

    // encrypted payload
    memcpy(&mpdu_out[pos], payload, len);
    mac_encrypt(ctx, counter, &mpdu_out[pos], len);
    pos += len;

    // SIGN over CIV | header | address | MPDUCNT | ciphertext (encrypt-then-MAC)
    mac_sign(ctx, counter, mpdu_out, pos, &mpdu_out[pos]);
    pos += MAC_SIGN_SIZE;

    ctx->packet_counter++;            // never reuse a counter (keystream must be unique)
    return pos;
}
