#include "phy.h"
#include "bit_access.h"
#include "radio_burst.h"


uint16_t phy_encode(radio_burst_t* const bursts, const uint8_t* const mpdu, uint8_t mpdu_len){

    //making buffer arrays for the PHY payload and the coded bits
    uint8_t phy_payload[TSUNBPHY_MAX_PSDU_LENGTH + TSUNBPHY_OVERHEAD];
    uint8_t coded[TSUNBPHY_MAX_FEC_BYTES];

    //build the PHY payload from the MPDU
    uint16_t payload_bits = phy_build_frame(mpdu, mpdu_len, phy_payload);
    phy_whiten_data(phy_payload, payload_bits);
    uint16_t coded_bits = phy_fec_encode(phy_payload, payload_bits, coded);

    phy_interleave(coded, coded_bits, bursts);

    uint16_t num_bursts = phy_num_bursts(mpdu_len);

    for(uint16_t i = 0; i < num_bursts; i++){
        radio_burst_add_pilot(&bursts[i], i);
        radio_burst_msk_precode(&bursts[i]);
    }

    return num_bursts;
}





uint16_t phy_build_frame(const uint8_t* const mpdu, uint8_t mpdu_len, uint8_t* const phy_payload){
    phy_payload[TSUNBPHY_PAYLOAD_PSI_POS] = mpdu_len; //set PSI field
    
    for(uint16_t i = 0; i < mpdu_len; i++){
        phy_payload[TSUNBPHY_PAYLOAD_DATA_POS + i] = mpdu[i]; //copy MPDU into payload
    }

    phy_payload[TSUNBPHY_PAYLOAD_DATA_POS + mpdu_len] = TSUNBPHY_MMODE_DUMMY << 6; //set MMODE field to dummy value

    phy_payload[TSUNBPHY_PAYLOAD_CRC_POS] = phy_crc8(&phy_payload[TSUNBPHY_PAYLOAD_DATA_POS], mpdu_len * 8 + TSUNBPHY_MMODE_BITS); //calculate payload CRC

    if(mpdu_len < TSUNBPHY_MIN_PSDU_LENGTH){
        for(uint16_t i = mpdu_len; i < TSUNBPHY_MIN_PSDU_LENGTH; i++){
            phy_payload[TSUNBPHY_PAYLOAD_DATA_POS + i] = 0; //zero pad payload to minimum length
        }
        phy_payload[TSUNBPHY_PAYLOAD_DATA_POS + TSUNBPHY_MIN_PSDU_LENGTH] = TSUNBPHY_MMODE_DUMMY << 6;
    }


    phy_payload[TSUNBPHY_HEADER_CRC_POS] = phy_crc8(&phy_payload[TSUNBPHY_PAYLOAD_CRC_POS],TSUNBPHY_PAYLOAD_CRC_BITS + TSUNBPHY_PAYLOAD_PSI_BITS ); //calculate header CRC

    uint16_t mpdu_total_bits = mpdu_len < TSUNBPHY_MIN_PSDU_LENGTH ? (TSUNBPHY_MIN_PSDU_LENGTH) * 8 : mpdu_len * 8;
    return TSUNBPHY_HEADER_BITS + mpdu_total_bits + TSUNBPHY_MMODE_BITS; //return total number of bits in the PHY payload
}


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

void phy_interleave(const uint8_t* const coded, uint16_t num_bits, radio_burst_t* const bursts){

    uint16_t num_bursts = num_bits / TSUNBPHY_DATABITS_BURST;
    uint8_t count[TSUNBPHY_MAX_BURSTS] = {0};

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
        bit_write(bursts[s].data,m, bit_read(coded, src));
    }
}


uint16_t phy_num_bursts(uint8_t mpdu_len){
    uint16_t payload_bytes = mpdu_len < TSUNBPHY_MIN_PSDU_LENGTH ? TSUNBPHY_MIN_PSDU_LENGTH : mpdu_len;
    return payload_bytes + TSUNBPHY_OVERHEAD; //one PHY payload byte becomes one burst after FEC
}

