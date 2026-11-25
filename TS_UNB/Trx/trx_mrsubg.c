#include "trx_mrsubg.h"
#include "tsma.h"              /* TSUNBTSMA_FC_HZ */
#include "stm32wl3x_hal.h"     /* MRSUBG HAL: SMRSubGConfig_t, HAL_MRSubG_Init, ... */
#include "phy.h"

void trx_mrsubg_init(void){
    SMRSubGConfig_t cfg = {0};

    // 1) radio parameters
    cfg.lFrequencyBase    = TSUNBTSMA_FC_HZ;   // center frequency for eu band 868.18 MHz, see ETSI TS 103 357-2, p.96
    cfg.xModulationSelect = MOD_2GFSK1;   // gaussian msk
    cfg.lDatarate         = TSUNBTRX_SYMBOL_RATE;      // symbols per second (sps), integer part of 2380.371, only used for setting E part
    cfg.lFreqDev          = TSUNBTRX_FREQ_DEV_HZ;
    cfg.lBandwidth        = TSUNBTRX_CHANNEL_BW_HZ;
    cfg.dsssExp           = 0;           // no spreading
    cfg.outputPower       = TSUNBTRX_TX_POWER_DBM;
    cfg.PADrvMode         = PA_DRV_TX_HP;
    HAL_MRSubG_Init(&cfg);


    // the HAL only takes integer sps (2380 -> -150 ppm, ETSI allows +-100 ppm).
    // Write the mantissa for 2380.371 sps directly:
    // M = 2380.371 * 2^33 / (16 MHz * 2^4) - 65536 = 14336
    MODIFY_REG_FIELD(MR_SUBG_GLOB_DYNAMIC->MOD0_CONFIG,MR_SUBG_GLOB_DYNAMIC_MOD0_CONFIG_DATARATE_M, TSUNBTRX_DATARATE_M);
    // raw bit stream: no preamble, sync or CRC added by the radio
    __HAL_MRSUBG_SET_TX_MODE(TX_DIRECT_BUFFERS);
    // let the sequencer report TX done and abort done (polled, no NVIC interrupt)
    __HAL_MRSUBG_SET_RFSEQ_IRQ_ENABLE(MR_SUBG_GLOB_DYNAMIC_RFSEQ_IRQ_ENABLE_TX_DONE_E |
                                      MR_SUBG_GLOB_DYNAMIC_RFSEQ_IRQ_ENABLE_SABORT_DONE_E);

}


/* ---- Timing ------------------------------------------------------------ */

/**
 * @brief  Convert a number of TS-UNB symbols to microseconds (rounded).
 * @param  symbols  Number of symbol periods
 * @retval Time in microseconds
 */
static uint32_t trx_symbols_to_us(uint32_t symbols){
    // 1 symbol = 1e6 / 2380.371 us = 1e9 / 2380371 us
    uint64_t num = (uint64_t)symbols * 1000000000ULL;
    return (uint32_t)((num + TSUNBTRX_SYMBOL_RATE_MILLI / 2) / TSUNBTRX_SYMBOL_RATE_MILLI);
}

/* software extension of the 16-bit TIM2 counter to 32 bits */
static uint16_t timer_last_cnt;
static uint32_t timer_us;

/**
 * @brief  Start TIM2 as a free-running 1 us counter and reset the time to 0.
 */
static void trx_timer_start(void){
    LL_APB0_GRP1_EnableClock(LL_APB0_GRP1_PERIPH_TIM2);   // clock to the timer

    TIM2->CR1 = 0;                                        // stop while configuring
    TIM2->PSC = (SystemCoreClock / TSUNBTRX_TIMER_HZ) - 1;  // prescaler for 1 MHz timer clock
    TIM2->ARR = 0xFFFF;                                   // count the full 16-bit range
    TIM2->EGR = TIM_EGR_UG;                               // load PSC now (it is buffered)
    TIM2->CNT = 0;

    timer_last_cnt = 0;
    timer_us       = 0;

    TIM2->CR1 = TIM_CR1_CEN;                              // start counting
}

/**
 * @brief  Time since trx_timer_start() in us. Must be called at least every 65 ms.
 */
static uint32_t trx_timer_now_us(void){
    uint16_t cnt = (uint16_t)TIM2->CNT;
    timer_us      += (uint16_t)(cnt - timer_last_cnt);  // works even if the counter wrapped
    timer_last_cnt = cnt;
    return timer_us;
}

/**
 * @brief  Busy-wait until the timer reaches t_us.
 */
static void trx_wait_until_us(uint32_t t_us){
    while (trx_timer_now_us() < t_us) { }
}


/* ---- Bursts ------------------------------------------------------------ */

/* Burst data must be in RAM and 4-byte aligned for the MRSUBG DMA */
static uint8_t tx_buf[8] __attribute__((aligned(4)));

/**
 * @brief  Load burst data and frequency into the radio, without starting TX.
 */
static void trx_prepare_burst(const radio_burst_t* const burst){
    // copy the 36 symbols (5 bytes) into the aligned TX buffer
    for (uint8_t i = 0; i < sizeof(burst->data) ; i++) {
        tx_buf[i] = burst->data[i];
    }

    // carrier frequency of this burst
    HAL_MRSubG_SetFrequencyBase(burst->freq_hz);

    // tell the radio where the data is and how many bytes to send
    __HAL_MRSUBG_SET_DATABUFFER0_POINTER((uint32_t)tx_buf);
    __HAL_MRSUBG_SET_DATABUFFER_SIZE(sizeof(burst->data));
    __HAL_MRSUBG_SET_PKT_LEN(sizeof(burst->data));

    // clear old flags, so an old TX_DONE cannot end the wait too early
    __HAL_MRSUBG_CLEAR_RFSEQ_IRQ_FLAG(0xFFFFFFFF);
}

/**
 * @brief  Transmit the prepared burst for exactly 36 symbols.
 *         The radio sends whole bytes (40 bits), so TX is aborted after symbol 36.
 */
static void trx_start_burst(void){
    uint32_t t0 = trx_timer_now_us();
    __HAL_MRSUBG_STROBE_CMD(CMD_TX);

    // wait until exactly 36 symbols have been sent, then stop the radio
    trx_wait_until_us(t0 + TSUNBTRX_TX_STARTUP_US + trx_symbols_to_us(TSUNBPHY_TOTAL_SYMBOLS_BURST));
    __HAL_MRSUBG_STROBE_CMD(CMD_SABORT);

    // wait until the abort is done (the PA ramps down)
    while ((MR_SUBG_GLOB_STATUS_RFSEQ_IRQ_STATUS_SABORT_DONE_F & __HAL_MRSUBG_GET_RFSEQ_IRQ_STATUS()) == 0) { }

    // clear all flags (SABORT_DONE and a possible TX_DONE)
    __HAL_MRSUBG_CLEAR_RFSEQ_IRQ_FLAG(0xFFFFFFFF);
}


uint8_t trx_mrsubg_transmit(const radio_burst_t* const bursts, uint16_t num_bursts){
    uint32_t start_sym = 0;                     // start of burst s, in symbols from burst 0

    trx_timer_start();

    for (uint16_t s = 0; s < num_bursts; s++) {
        trx_prepare_burst(&bursts[s]);
        
        // absolute start time of burst s, so delays cannot add up over the frame
        trx_wait_until_us(TSUNBTRX_FRAME_LEAD_US + trx_symbols_to_us(start_sym));

        trx_start_burst();

        start_sym += bursts[s].t_rb;            // last burst has t_rb = 0
    }
    return 0;
}




