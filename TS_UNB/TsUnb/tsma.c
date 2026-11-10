#include "tsma.h"
#include "phy.h"

/*
 * Uplink TSMA Pattern Group 1 (UPG1), ETSI TS 103 357-2 p.102.
 * Row = pattern p (row 0 is p = 1 in the standard), column = sub-packet (burst) s.
 */

/* Radio-burst carrier C_RB(s), Table 4-108. Each row is a permutation of carriers 0..23. */
static const uint8_t tsma_c_rb[TSUNBTSMA_PATTERNS][TSUNBPHY_CORE_FRAME_BURSTS] = {
    { 5, 21, 13,  6, 22, 14,  1, 17,  9,  0, 16,  8,  7, 23, 15,  4, 20, 12,  3, 19, 11,  2, 18, 10},   /* p = 1 */
    { 4, 20, 12,  1, 17,  9,  0, 16,  8,  6, 22, 14,  7, 23, 15,  2, 18, 10,  5, 21, 13,  3, 19, 11},   /* p = 2 */
    { 4, 20, 12,  3, 19, 11,  6, 22, 14,  7, 23, 15,  0, 16,  8,  5, 21, 13,  2, 18, 10,  1, 17,  9},   /* p = 3 */
    { 6, 22, 14,  2, 18, 10,  7, 23, 15,  0, 16,  8,  1, 17,  9,  4, 20, 12,  5, 21, 13,  3, 19, 11},   /* p = 4 */
    { 7, 23, 15,  4, 20, 12,  3, 19, 11,  2, 18, 10,  6, 22, 14,  0, 16,  8,  1, 17,  9,  5, 21, 13},   /* p = 5 */
    { 3, 19, 11,  6, 22, 14,  2, 18, 10,  0, 16,  8,  7, 23, 15,  1, 17,  9,  4, 20, 12,  5, 21, 13},   /* p = 6 */
    { 3, 19, 11,  1, 17,  9,  5, 21, 13,  7, 23, 15,  0, 16,  8,  2, 18, 10,  6, 22, 14,  4, 20, 12},   /* p = 7 */
    { 0, 16,  8,  6, 22, 14,  3, 19, 11,  2, 18, 10,  4, 20, 12,  7, 23, 15,  5, 21, 13,  1, 17,  9},   /* p = 8 */
};

/*
 * Radio-burst time T_RB(s) in symbols, Table 4-109.
 * The standard numbers these s = 1..23: T_RB(s) is the spacing from burst s-1 to burst s.
 * So column k here is the time from burst k to burst k+1 (23 gaps between 24 bursts).
 */
static const uint16_t tsma_t_rb[TSUNBTSMA_PATTERNS][TSUNBPHY_CORE_FRAME_BURSTS - 1] = {
    {330, 387, 388, 330, 387, 354, 330, 387, 356, 330, 387, 432, 330, 387, 352, 330, 387, 467, 330, 387, 620, 330, 387},   /* p = 1 */
    {330, 387, 435, 330, 387, 409, 330, 387, 398, 330, 387, 370, 330, 387, 361, 330, 387, 472, 330, 387, 522, 330, 387},   /* p = 2 */
    {330, 387, 356, 330, 387, 439, 330, 387, 413, 330, 387, 352, 330, 387, 485, 330, 387, 397, 330, 387, 444, 330, 387},   /* p = 3 */
    {330, 387, 352, 330, 387, 382, 330, 387, 381, 330, 387, 365, 330, 387, 595, 330, 387, 604, 330, 387, 352, 330, 387},   /* p = 4 */
    {330, 387, 380, 330, 387, 634, 330, 387, 360, 330, 387, 393, 330, 387, 352, 330, 387, 373, 330, 387, 490, 330, 387},   /* p = 5 */
    {330, 387, 364, 330, 387, 375, 330, 387, 474, 330, 387, 355, 330, 387, 478, 330, 387, 464, 330, 387, 513, 330, 387},   /* p = 6 */
    {330, 387, 472, 330, 387, 546, 330, 387, 501, 330, 387, 356, 330, 387, 359, 330, 387, 359, 330, 387, 364, 330, 387},   /* p = 7 */
    {330, 387, 391, 330, 387, 468, 330, 387, 512, 330, 387, 543, 330, 387, 354, 330, 387, 391, 330, 387, 368, 330, 387},   /* p = 8 */
};

/**
 * @brief  Pseudo-random carrier offset C_RF for a radio frame (ETSI p.96, Table 4-105).
 * @param  payload_crc  Payload CRC, before whitening
 * @retval C_RF in the range -(n_co/2) .. +(n_co/2), i.e. -1, 0 or +1 for n_co = 3
 */
int8_t tsma_carrier_offset(uint8_t payload_crc){
    
    uint8_t v_co = payload_crc >> 1;
   
    return (int8_t)(v_co % TSUNBTSMA_N_CO) - (TSUNBTSMA_N_CO / 2);
}

/**
 * @brief  Calculate the frequency of a radio burst s.96
 * @param  c_rb  Carrier index (0..23)
 * @param  c_rf  Radio frequency offset (-1, 0 or +1 for n_co = 3)
 * @retval Burst frequency in Hz
 */
uint32_t tsma_burst_freq(uint8_t c_rb, int8_t c_rf){
    // Implementation for calculating burst frequency
    int32_t steps = (int32_t)(c_rb - TSUNBTSMA_CENTER_CARRIER + c_rf);
    int32_t freq_offset_mhz = steps * TSUNBTSMA_B_C0_MILLIHZ; // in milli Hz

    int32_t freq_offset_hz = freq_offset_mhz >= 0 ? (freq_offset_mhz + 500) / 1000 : (freq_offset_mhz - 500) / 1000; // rounding to nearest Hz

    return (int32_t)TSUNBTSMA_FC_HZ + freq_offset_hz;
}

/**
 * @brief  Apply an uplink TSMA pattern (UPG1) to the radio bursts of a frame (ETSI 4.7.6.1).
 * @param  bursts       Radio bursts from phy_encode()
 * @param  num_bursts   Number of bursts in the frame
 * @param  pattern      TSMA pattern 0..7 (p = 1..8 in the standard)
 * @param  payload_crc  Payload CRC, before whitening
 */
void tsma_apply(radio_burst_t* const bursts, uint16_t num_bursts,
                uint8_t pattern, uint8_t payload_crc){

    if (pattern >= TSUNBTSMA_PATTERNS) {
        return; //her kan man adde en fejlkode hvis det er
    }
    int8_t c_rf = tsma_carrier_offset(payload_crc);

    //this is for the core frame
    for (uint8_t s = 0; s < TSUNBPHY_CORE_FRAME_BURSTS; s++) {
        bursts[s].freq_hz = tsma_burst_freq(tsma_c_rb[pattern][s], c_rf);

        if (s < TSUNBPHY_CORE_FRAME_BURSTS - 1) {
            bursts[s].t_rb = tsma_t_rb[pattern][s];
        }
    }

    // TODO: extension frame (bursts 24..num_bursts-1) uses the LFSR, ETSI 4.7.6.1.7

    //the last burst of the frame has no next burst
    bursts[num_bursts - 1].t_rb = 0;
}
