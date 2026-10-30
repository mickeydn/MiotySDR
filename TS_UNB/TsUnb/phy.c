#include "phy.h"
#include "bit_access.h"
#include "burst_helpers.h"


uint8_t phy_crc8(const uint8_t* const data, uint16_t num_bits){
    uint8_t crc = TSUNBPHY_CRC8_INIT;

    for (uint16_t bit_idx = 0; bit_idx < num_bits; bit_idx++) {
        uint8_t feedback = ((crc >> 7) & 0x01) ^ bit_read(data, bit_idx);

        crc <<= 1;
        if (feedback) {
            crc ^= TSUNBPHY_CRC8_POLY;
        }
    }

    return crc;
}

// foelger figuren paa IEEE starndardens dokument
void phy_whiten_data(uint8_t* const data, uint16_t num_bits){
    uint16_t whiten_reg = TSUNBPHY_WHITEN_INIT;

    for(uint16_t bit_idx = 0; bit_idx < num_bits; bit_idx++) {
            //foerste skridt, XOR bit 0 med bit 5, og gem i feedback
           uint8_t feedback = 0x01 & (whiten_reg ^ (whiten_reg >> 5));
           whiten_reg = (whiten_reg >> 1) | (feedback << 8);
           //find databit, XOR med feedback 
           uint8_t data_bit = bit_read(data, bit_idx);
           uint8_t whitened_bit = data_bit ^ feedback;
            //fyld det i data arrayet igen
           bit_write(data, bit_idx, whitened_bit);
    }
        
}

uint16_t phy_fec_encode(const uint8_t* const input, uint16_t num_bits, uint8_t* const output){

    uint8_t reg = 0;

    for(uint16_t i = 0; i < num_bits + TSUNBPHY_FEC_TAIL_BITS; i++) {

        uint8_t input_bit = (i < num_bits) ? bit_read(input, i) : 0;

        reg = (reg >> 1) | (input_bit << 6);

        bit_write(output, i * TSUNBPHY_FEC_RATE + 0, bit_parity(reg & TSUNBPHY_FEC_POLY_G1));
        bit_write(output, i * TSUNBPHY_FEC_RATE + 1, bit_parity(reg & TSUNBPHY_FEC_POLY_G2));
        bit_write(output, i * TSUNBPHY_FEC_RATE + 2, bit_parity(reg & TSUNBPHY_FEC_POLY_G3));
        
    }

    return (num_bits + TSUNBPHY_FEC_TAIL_BITS) * TSUNBPHY_FEC_RATE;

}

void phy_interleave(const uint8_t* const coded, uint16_t num_bits, uint8_t* const bursts){

    uint16_t num_bursts = num_bits / TSUNBPHY_DATABITS_BURST;
    uint8_t count[TSUNBPHY_MAX_PSDU_LENGTH + TSUNBPHY_OVERHEAD] = {0};

    for (uint16_t i = 0; i < num_bits; i++) {

        //shifting 
        uint16_t src =  i + num_bits - TSUNBPHY_ILV_INIT_SHIFT;
        src = src >= num_bits ? src - num_bits : src;

        //mapper til subpacket og position
        uint16_t s = phy_ilv_subpacket(i, num_bursts);
        uint8_t  o = count[s];
        count[s]++;
        uint8_t  m = phy_ilv_position(s, o);

        // sætter 
        bit_write(bursts, s * TSUNBPHY_TOTAL_SYMBOLS_BURST + m, bit_read(coded, src));
    }
}


void phy_insert_pilot(uint8_t* const bursts, uint16_t num_bursts){

    static const uint8_t pilot_core[2] = {0x74, 0x20};
    static const uint8_t pilot_ext[2]  = {0x4F, 0xA0};

    for (uint16_t s = 0; s < num_bursts; s++) {

        const uint8_t* pilot = (s < TSUNBPHY_CORE_FRAME_BURSTS) ? pilot_core : pilot_ext;

        for (uint8_t k = 0; k < TSUNBPHY_PILOT_BITS; k++) {
            bit_write(bursts, s * TSUNBPHY_TOTAL_SYMBOLS_BURST + TSUNBPHY_SYMBOLS_PER_HALF_BURST + k, bit_read(pilot, k));
        }
    }
}
