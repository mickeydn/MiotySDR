#ifndef PHY_H          
#define PHY_H          

#include <stdint.h>    

#define TSUNBPHY_CRC8_POLY          0x9B

#define TSUNBPHY_CRC8_INIT			0xFF

#define TSUNBPHY_WHITEN_INIT		0x1FF

/* PHY payload fields (ETSI TS 103 357-2, Table 4-70) */
#define TSUNBPHY_HEADER_BITS        24      /* Header CRC + Payload CRC + PSI */
#define TSUNBPHY_MMODE_BITS         2
#define TSUNBPHY_MIN_PSDU_LENGTH    20      /* bytes, shorter MPDUs are zero padded */
#define TSUNBPHY_MAX_PSDU_LENGTH    255     /* bytes, PSI is 8 bits */

/* FEC (ETSI p.94): rate 1/3, constraint length 7, 6 zero tail bits */
#define TSUNBPHY_FEC_RATE           3
#define TSUNBPHY_FEC_TAIL_BITS      6
#define TSUNBPHY_FEC_POLY_G1        0x6D    /* 155 octal */
#define TSUNBPHY_FEC_POLY_G2        0x53    /* 123 octal */
#define TSUNBPHY_FEC_POLY_G3        0x5F    /* 137 octal */

#define TSUNBPHY_OVERHEAD           4       /* Header CRC, Payload CRC, PSI, MMODE + tail byte */
#define TSUNBPHY_MAX_FEC_BYTES      777     /* 3 * (TSUNBPHY_MAX_PSDU_LENGTH + TSUNBPHY_OVERHEAD) */
#define TSUNBPHY_DATABITS_BURST     24  
#define TSUNBPHY_PILOT_BITS         12

//interleaver defines
#define TSUNBPHY_CORE_FRAME_BURSTS  24
#define TSUNBPHY_FRAME_FIRST_HALF   288     //count of bits in first half of frame (24 * 12 bits)
#define TSUNBPHY_ILV_INIT_SHIFT     48      //bits shifted before interleaving
#define TSUNBPHY_TOTAL_SYMBOLS_BURST  36
#define TSUNBPHY_SYMBOLS_PER_HALF_BURST  12



// Function prototypes

uint8_t phy_crc8(const uint8_t* const data, uint16_t num_bits);   

void phy_whiten_data(uint8_t* const data, uint16_t num_bits);

uint16_t phy_fec_encode(const uint8_t* const input, uint16_t num_bits, uint8_t* const output);

void phy_interleave(const uint8_t* const coded, uint16_t num_bits, uint8_t* const bursts);

void phy_insert_pilot(uint8_t* const bursts, uint16_t num_bursts);



#endif
