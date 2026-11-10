#ifndef TSMA_H          
#define TSMA_H      

#include <stdint.h>
#include "radio_burst.h"

#define TSUNBTSMA_FC_HZ            868180000UL
#define TSUNBTSMA_B_C0_MILLIHZ             2380371 //carrier spacing in milli hz to avoid floating point operations
#define TSUNBTSMA_CENTER_CARRIER            12
#define TSUNBTSMA_CARRIER_COUNT    24     //number of carriers in a frame
#define TSUNBTSMA_N_CO              3
#define TSUNBTSMA_PATTERNS          8     //uplink pattern group 1


int8_t tsma_carrier_offset(uint8_t payload_crc);

uint32_t tsma_burst_freq(uint8_t c_rb, int8_t c_rf);

void tsma_apply(radio_burst_t* const bursts, uint16_t num_bursts,uint8_t pattern, uint8_t payload_crc);




#endif