/**
 * @file    burst_helpers.h
 * @brief   Helpers for distributing coded bits onto TS-UNB radio bursts.
 */
#ifndef BURST_HELPERS_H
#define BURST_HELPERS_H

#include <stdint.h>

/**
 * @brief  Find the sub-packet (radio burst) a coded bit belongs to.
 * @param  bit_idx     Index of the bit after the cyclic shift
 * @param  num_bursts  Total number of radio bursts (24 for a core frame)
 * @retval Sub-packet index s
 */
uint16_t phy_ilv_subpacket(uint16_t bit_idx, uint16_t num_bursts);

/**
 * @brief  Find the symbol position of a bit inside its radio burst.
 * @param  subpacket  Sub-packet index s
 * @param  rank       Number of bits already placed in this sub-packet (o)
 * @retval Symbol position m, 0..11 or 24..35
 */
uint8_t phy_ilv_position(uint16_t subpacket, uint8_t rank);

#endif /* BURST_HELPERS_H */
