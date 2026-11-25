/**
 * @file    trx_mrsubg.h
 * @brief   TS-UNB burst transmitter for the STM32WL33 MRSUBG radio.
 */
#ifndef TRX_MRSUBG_H
#define TRX_MRSUBG_H

#include <stdint.h>
#include "radio_burst.h"

/* Radio settings for TS-UNB standard mode (ETSI TS 103 357-2) */
#define TSUNBTRX_SYMBOL_RATE        2380            // symbols per second (sps), integer part of 2380.371
#define TSUNBTRX_FREQ_DEV_HZ        595             // MSK: symbol rate / 4
#define TSUNBTRX_TX_POWER_DBM       14            // EU0: max 14 dBm erp (regional profile)
#define TSUNBTRX_CHANNEL_BW_HZ      100000        // RX filter only, not used for TX 
#define TSUNBTRX_DATARATE_M         14336         // mantissa for 2380.371 sps at E = 4, see trx_mrsubg_init 
#define TSUNBTRX_SYMBOL_RATE_MILLI  2380371       // symbol rate in milli-sps (2380.371 sps)
#define TSUNBTRX_TIMER_HZ           1000000       // TIM2 tick rate: 1 tick = 1 us
#define TSUNBTRX_FRAME_LEAD_US      1000          // time from timer start to burst 0, room to prepare burst 0

#define TSUNBTRX_TX_STARTUP_US      1090          // CMD_TX -> start of symbol 1, incl. CMD_SABORT latency (tuned from SDR recording)

/**
 * @brief  Configure the MRSUBG for TS-UNB transmission (MSK, raw bit stream).
 */
void trx_mrsubg_init(void);

/**
 * @brief  Transmit the radio bursts of one TS-UNB frame.
 * @param  bursts      Radio bursts from phy_encode() (data, freq_hz, t_rb)
 * @param  num_bursts  Number of bursts
 * @retval 0 if OK
 */
uint8_t trx_mrsubg_transmit(const radio_burst_t* const bursts, uint16_t num_bursts);


#endif /* TRX_MRSUBG_H */
