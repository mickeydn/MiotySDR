/**
 * @file    bit_access.h
 * @brief   Read and write single bits in a byte array, MSB first.
 *
 * Bit index 0 is the MSB (bit 7) of data[0], index 7 is the LSB of data[0],
 * index 8 is the MSB of data[1], and so on. This is the bit order used by
 * TS-UNB (ETSI TS 103 357-2, p.22).
 */
#ifndef BIT_ACCESS_H
#define BIT_ACCESS_H

#include <stdint.h>

/**
 * @brief  Read one bit from a byte array.
 * @param  data     Byte array holding the bits
 * @param  bit_idx  Position of the bit, 0 = MSB of data[0]
 * @retval The bit value, 0 or 1
 */
uint8_t bit_read(const uint8_t *data, uint16_t bit_idx);

/**
 * @brief  Write one bit into a byte array, leaving the other bits unchanged.
 * @param  data     Byte array holding the bits
 * @param  bit_idx  Position of the bit, 0 = MSB of data[0]
 * @param  bit      Value to write; 0 clears the bit, anything else sets it
 */
void bit_write(uint8_t* const data, uint16_t bit_idx, uint8_t bit);

uint8_t bit_parity(uint8_t register_value);


#endif /* BIT_ACCESS_H */
