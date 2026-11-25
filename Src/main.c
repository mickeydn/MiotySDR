/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    main.c
  * @author  GPM WBL Application Team
  * @brief   Basic project structure template.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include "phy.h"
#include "tsma.h"
#include "mac.h"
#include "mac_keys.h"
#include "mac_nvm.h"
#include "stm32wl3x_nucleo.h"
#include "trx_mrsubg.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
  APP_MODE_TRIGGER,
  APP_MODE_AUTO
} app_mode_t;

typedef enum {
  PRESS_NONE,
  PRESS_SHORT,
  PRESS_LONG
} press_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define APP_AUTO_INTERVAL_MS   40000              // time between uplinks in auto mode
#define APP_LONG_PRESS_MS      1000               // B1 held this long = class A uplink
#define APP_AUTO_UPLINK_FLAGS  MAC_FLAGS_CLASS_Z  // uplink type used in auto mode

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static radio_burst_t bursts[TSUNBPHY_MAX_BURSTS];
static uint16_t num_bursts;
static mac_ctx_t mac_ctx;

// application payload (LPDU), max 10 bytes with short addressing
static const uint8_t app_payload[] = { 'M', 'I', 'O', 'T', 'Y' };
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_GPIO_Init(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static uint8_t button_pressed(Button_TypeDef b){
    static uint8_t last[BUTTONn];          // previous state per button
    uint8_t now = (uint8_t)BSP_PB_GetState(b);
    uint8_t edge = (now == 1 && last[b] == 0);
    last[b] = now;
    return edge;
}

/**
 * @brief  Classify presses on B1: short press (reported on release) or
 *         long press (reported once when held for APP_LONG_PRESS_MS).
 */
static press_t b1_press_type(void){
    static uint8_t  down;
    static uint8_t  long_reported;
    static uint32_t t_down;
    uint8_t now = (uint8_t)BSP_PB_GetState(B1);
    press_t result = PRESS_NONE;

    if (now && !down){                                   // pressed
        down = 1;
        long_reported = 0;
        t_down = HAL_GetTick();
    } else if (now && down && !long_reported &&
               (HAL_GetTick() - t_down) >= APP_LONG_PRESS_MS){   // held long enough
        long_reported = 1;
        result = PRESS_LONG;
    } else if (!now && down){                            // released
        down = 0;
        if (!long_reported){
            result = PRESS_SHORT;
        }
    }
    return result;
}

static void app_mac_init(void){
    const uint8_t eui[8]  = MAC_KEY_EUI64;
    const uint8_t key[16] = MAC_KEY_NWK_KEY;
    memcpy(mac_ctx.eui64, eui, sizeof(eui));
    memcpy(mac_ctx.nwk_key, key, sizeof(key));
    mac_ctx.short_addr     = MAC_KEY_SHORT_ADDR;
    mac_ctx.use_short_addr = MAC_KEY_USE_SHORT;
    mac_ctx.packet_counter = mac_nvm_load_counter(MAC_KEY_START_COUNTER);
}

/**
 * @brief  Build and transmit one uplink: MAC -> store counter -> PHY -> radio.
 * @param  flags  MAC_FLAGS_CLASS_Z, or MAC_FLAGS_CLASS_A to request a downlink
 */
static void send_uplink(uint8_t flags){
    uint8_t mpdu[MAC_MAX_MPDU_LEN];
    uint32_t counter = mac_ctx.packet_counter;           // counter used by this packet

    uint8_t mpdu_len = mac_build_uplink(&mac_ctx, app_payload, sizeof(app_payload), flags, mpdu);
    if (mpdu_len == 0){
        return;
    }
    // store the next counter before transmitting, so a counter is never reused after a reset
    // (also keeps the flash erase out of the frame timing)
    mac_nvm_save_counter(mac_ctx.packet_counter);

    // vary the TSMA pattern from frame to frame (ETSI: use at least 6 of the 8 UPG1 patterns)
    uint8_t pattern = (uint8_t)(counter % TSUNBTSMA_PATTERNS);
    num_bursts = phy_encode(bursts, mpdu, mpdu_len, pattern);
    if (num_bursts == 0){
        return;
    }

    BSP_LED_On(LED_BLUE);                                // blue: transmitting
    trx_mrsubg_transmit(bursts, num_bursts);
    BSP_LED_Off(LED_BLUE);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();

  /* USER CODE BEGIN 2 */
  trx_mrsubg_init();
  BSP_PB_Init(B1, BUTTON_MODE_GPIO);
  BSP_PB_Init(B2, BUTTON_MODE_GPIO);
  BSP_LED_Init(LED_BLUE);
  BSP_LED_Init(LED_GREEN);
  BSP_LED_Init(LED_RED);
  BSP_LED_Off(LED_BLUE);
  BSP_LED_Off(LED_GREEN);
  BSP_LED_On(LED_RED);            // start in trigger mode

  app_mac_init();

  app_mode_t app_mode = APP_MODE_TRIGGER;
  uint32_t last_tx = 0;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE BEGIN 3 */
    /* USER CODE END WHILE */
   

    // B2 toggles between trigger mode (red LED) and auto mode (green LED)
    if (button_pressed(B2)){
      if (app_mode == APP_MODE_TRIGGER){
        app_mode = APP_MODE_AUTO;
        BSP_LED_On(LED_GREEN);
        BSP_LED_Off(LED_RED);
        last_tx = HAL_GetTick() - APP_AUTO_INTERVAL_MS;   // first auto uplink right away
      } else {
        app_mode = APP_MODE_TRIGGER;
        BSP_LED_On(LED_RED);
        BSP_LED_Off(LED_GREEN);
      }
    }

    // B1 in trigger mode: short press = class Z uplink, long press = class A uplink (requests a downlink)
    press_t b1 = b1_press_type();
    if (app_mode == APP_MODE_TRIGGER && b1 == PRESS_SHORT) {
        send_uplink(MAC_FLAGS_CLASS_Z);
    }
    if (app_mode == APP_MODE_TRIGGER && b1 == PRESS_LONG) {
        send_uplink(MAC_FLAGS_CLASS_A);
    }
    if (app_mode == APP_MODE_AUTO && (HAL_GetTick() - last_tx) >= APP_AUTO_INTERVAL_MS) {
        last_tx = HAL_GetTick();
        send_uplink(APP_AUTO_UPLINK_FLAGS);
    }

    HAL_Delay(10);                          // simple debounce: poll every 10 ms
    /* USER CODE END 3 */
}
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the SYSCLKSource and SYSCLKDivider
  */
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_RC64MPLL;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_RC64MPLL_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_WAIT_STATES_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SMPS;
  PeriphClkInitStruct.SmpsDivSelection = RCC_SMPSCLK_DIV4;
  PeriphClkInitStruct.KRMRateMultiplier = 4;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }

  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
