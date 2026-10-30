/**
 * @file    bit_access.c
 * @brief   Read and write single bits in a byte array, MSB first.
 */
#include "bit_access.h"

uint8_t bit_read(const uint8_t *data, uint16_t bit_idx)
{
    uint16_t byte_idx = bit_idx / 8;
    uint8_t bit_pos = 7 - (bit_idx % 8);
    return (data[byte_idx] >> bit_pos) & 0x01;
}

void bit_write(uint8_t* const data, uint16_t bit_idx, uint8_t bit)
{

    uint16_t byte_idx = bit_idx / 8;
    uint8_t bit_pos = 7 - (bit_idx % 8);
    uint8_t mask = 1 << bit_pos;

    if (bit) {
        data[byte_idx] |= mask;
    } else {
        data[byte_idx] &= ~mask;
    }
}

uint8_t bit_parity(uint8_t register_value){
    register_value ^= register_value >> 4;
    register_value ^= register_value >> 2;
	register_value ^= register_value >> 1;

	return register_value & 0x01;
}