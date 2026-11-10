/**
 * @file    radio_burst.h
 * @brief   Helpers for distributing coded bits onto TS-UNB radio bursts.
 */
#ifndef RADIO_BURST_H
#define RADIO_BURST_H

#include <stdint.h>


typedef struct{
    uint32_t freq_hz;   //frequency of the burst in Hz
    uint8_t data[5];    //5 bytes = 40 bits, enough for 36 bits of a burst
    uint16_t t_rb;      //time of burst delay in symbols
} radio_burst_t;

void radio_burst_msk_precode(radio_burst_t* const burst);

void radio_burst_add_pilot(radio_burst_t* const burst, uint16_t burst_idx);

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



#endif /* RADIO_BURST_H */
