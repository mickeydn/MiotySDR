/*
 * TSMA-style transmitter for STM32WL33 (based on MRSUBG_BasicGeneric_Tx)
 *
 * Sends one telegram as 24 short bursts following Uplink TSMA Pattern
 * Group 1, pattern 1 only (Table 4-108 for carriers, Table 4-109 for timing).
 *
 * ASSUMPTIONS to verify in the standard's text:
 *   - the time unit in Table 4-109 is one symbol period (1 / SYMBOL_RATE_HZ)
 *   - T_RB(s) is measured from the start of burst s-1 to the start of burst s
 *   - carrier spacing and base frequency of carrier C0
 */

#include <stdint.h>
#include "main.h"   /* project header: pulls in the HAL and MRSUBG types */

/* Radio configuration structs defined in main.c (MX_MRSUBG_Init) */
extern SMRSubGConfig_t MRSUBG_RadioInitStruct;
extern MRSubG_PcktBasicFields_t MRSUBG_PacketSettingsStruct;

#define N_BURSTS            24U
#define BASE_FREQ_HZ        868000000UL   /* frequency of carrier C0 (adjust, see standard) */
#define CARRIER_SPACING_HZ  2380.371f     /* check exact value in the standard */
#define SYMBOL_RATE_HZ      2380.371f

#define TEST_DATA_BITS      0xA5CU
#define CORE_PILOT          0x742U



/* Table 4-108, pattern 1: carrier number of burst s (s = 0..23) */
static const uint8_t carrier_of_burst[N_BURSTS] = {
     5, 21, 13,  6, 22, 14,  1, 17,  9,  0, 16,  8,
     7, 23, 15,  4, 20, 12,  3, 19, 11,  2, 18, 10,
};

/* Table 4-109, pattern 1: T_RB(s) for s = 1..23, in symbol periods.
 * Burst 0 starts at t = 0. */
static const uint16_t dt_symbols[N_BURSTS - 1U] = {
    330, 387, 388, 330, 387, 354, 330, 387, 356, 330, 387, 432,
    330, 387, 352, 330, 387, 467, 330, 387, 620, 330, 387,
};

/* ---- Radio hooks, using the same calls as MRSUBG_BasicGeneric_Tx ------- */
#define BURST_PAYLOAD_LEN 5U
#define BURST_LENGTH_BITS   36U

/* The radio reads the payload from this buffer (must be 4-byte aligned) */
__attribute__((aligned(4))) static uint8_t tx_buf[BURST_PAYLOAD_LEN];

static void radio_set_frequency_hz(uint32_t freq_hz)
{
    /* Simple and safe approach: change the base frequency and re-initialise.
     * Later you can look in the MRSUBG HAL header for a lighter
     * "set frequency" function to make the hop faster. */
    MRSUBG_RadioInitStruct.lFrequencyBase = freq_hz;
    HAL_MRSubG_Init(&MRSUBG_RadioInitStruct);
    HAL_MRSubG_PacketBasicInit(&MRSUBG_PacketSettingsStruct);

    /* Re-apply the settings main() does once, in case Init reset them */
    HAL_MRSubG_PktBasicSetPayloadLength(BURST_PAYLOAD_LEN);
    __HAL_MRSUBG_SET_TX_MODE(TX_NORMAL);
    __HAL_MRSUBG_SET_DATABUFFER0_POINTER((uint32_t)tx_buf);
}

static void radio_send(const uint8_t *data, uint8_t len)
{
    for (uint8_t i = 0; i < len && i < BURST_PAYLOAD_LEN; i++) {
        tx_buf[i] = data[i];
    }
    __HAL_MRSUBG_STROBE_CMD(CMD_TX);
}

static void radio_wait_tx_done(void)
{
    while ((__HAL_MRSUBG_GET_RFSEQ_IRQ_STATUS() &
            MR_SUBG_GLOB_STATUS_RFSEQ_IRQ_STATUS_TX_DONE_F) == 0) {}

    __HAL_MRSUBG_CLEAR_RFSEQ_IRQ_FLAG(MR_SUBG_GLOB_STATUS_RFSEQ_IRQ_STATUS_TX_DONE_F);
}

/* ---- Helpers ---------------------------------------------------------- */
static uint32_t carrier_to_freq_hz(uint8_t carrier)
{
    return BASE_FREQ_HZ + (uint32_t)(carrier * CARRIER_SPACING_HZ + 0.5f);
}

/* Short payload per burst: message id, burst index, two data bytes. */
static void build_burst_payload(uint8_t out[BURST_PAYLOAD_LEN])
{
    // Build the complete 36-bit sequence:
    // 12 data bits + 12 pilot bits + 12 data bits
    uint64_t bitstream =
        ((uint64_t)TEST_DATA_BITS << 24) |
        ((uint64_t)CORE_PILOT << 12) |
        (uint64_t)TEST_DATA_BITS;

    // Clear the output buffer
    for (uint8_t i = 0; i < BURST_PAYLOAD_LEN; i++)
    {
        out[i] = 0;
    }

    // Pack the 36 bits MSB-first
    for (uint8_t i = 0; i < BURST_LENGTH_BITS; i++)
    {
        uint8_t bit = (bitstream >> (35U - i)) & 0x01U;

        out[i / 8U] |= bit << (7U - (i % 8U));
    }
}

/* ---- Send one telegram ------------------------------------------------ */
void tsma_send_telegram(void)
{
    uint32_t t0 = HAL_GetTick();
    uint32_t sym_acc = 0;

    for (uint8_t i = 0; i < N_BURSTS; i++)
    {
        if (i > 0)
            sym_acc += dt_symbols[i - 1];

        uint32_t start_ms =
            (uint32_t)(sym_acc * (1000.0f / SYMBOL_RATE_HZ));

        while ((HAL_GetTick() - t0) < start_ms) {}

        uint8_t payload[BURST_PAYLOAD_LEN];

        build_burst_payload(payload);

        radio_set_frequency_hz(
            carrier_to_freq_hz(carrier_of_burst[i])
        );

        radio_send(payload, BURST_PAYLOAD_LEN);
        radio_wait_tx_done();
    }
}

/* ---- Example use in main.c ---------------------------------------------
 *
 * In main.c, add near the top (USER CODE BEGIN PV):
 *     void tsma_send_telegram(uint8_t msg_id, const uint8_t msg[48]);
 *     static uint8_t message[48];
 *     static uint8_t msg_id = 0;
 *
 * and replace the body of the while(1) loop with:
 *     BSP_LED_On(LD2);
 *     tsma_send_telegram(msg_id++, message);
 *     BSP_LED_Off(LD2);
 *     HAL_Delay(2000);
 *
 * Do not use printf between bursts: the UART is slow and breaks the timing.
 */