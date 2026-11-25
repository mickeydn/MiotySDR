#ifndef MAC_H
#define MAC_H

#include <stdint.h>

/* UL MAC header bits (ETSI TS 103 357-2 Table 4-48, MSB = #0) */
#define MAC_HDR_ACK            0x01   // EP received a DL since its last UL
#define MAC_HDR_ATTACH         0x02   // attach packet (not supported)
#define MAC_HDR_ADDR_EUI64     0x04   // 1 = 64-bit EUI on air, 0 = 16-bit short address
#define MAC_HDR_RX_OPEN        0x08   // EP opens a DL RX window after this UL
#define MAC_HDR_RESPONSE       0x10   // EP requests a DL response
#define MAC_HDR_CONTROL        0x20   // LPDU starts with control segments (not supported)
#define MAC_HDR_MPF            0x40   // MPF byte present (not supported)

/* flags the application may pass to mac_build_uplink */
#define MAC_FLAGS_CLASS_Z      0x00
#define MAC_FLAGS_CLASS_A      (MAC_HDR_RX_OPEN | MAC_HDR_RESPONSE)   // request a downlink

#define MAC_SIGN_SIZE          4
#define MAC_COUNTER_SIZE       3      // MPDUCNT = 24 LSB of the packet counter
#define MAC_MAX_MPDU_LEN       20     // core frame only (no extension frame yet)

typedef struct {
    uint8_t  eui64[8];
    uint8_t  nwk_key[16];
    uint16_t short_addr;
    uint8_t  use_short_addr;          // 1 = short address on air, 0 = EUI64 on air
    uint32_t packet_counter;          // ext(8) | MPDUCNT(24), counter of the NEXT packet
} mac_ctx_t;

/**
 * @brief  Build a fixed-MAC uplink MPDU (encrypt payload, append SIGN) and
 *         advance the packet counter.
 * @param  ctx       device configuration and packet counter
 * @param  payload   user data (LPDU), 1..len bytes
 * @param  len       payload length; must fit in MAC_MAX_MPDU_LEN with the overhead
 *                   (short address: max 10 B, EUI64: max 4 B)
 * @param  flags     MAC_FLAGS_CLASS_Z or MAC_FLAGS_CLASS_A (only response/RX-open bits are used)
 * @param  mpdu_out  output buffer, at least MAC_MAX_MPDU_LEN bytes
 * @retval MPDU length, or 0 if the payload is empty or too long
 */
uint8_t mac_build_uplink(mac_ctx_t *ctx, const uint8_t *payload, uint8_t len,
                         uint8_t flags, uint8_t *mpdu_out);

#endif /* MAC_H */
