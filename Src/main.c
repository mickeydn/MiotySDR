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
#include "phy.h"
#include "stm32wl3x_nucleo.h"
#include "trx_mrsubg.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
  APP_MODE_TRIGGER,
  APP_MODE_AUTO
} app_mode_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define APP_AUTO_INTERVAL_MS   40000   // time between uplinks in auto mode
#define APP_TSMA_PATTERN       0       // fixed TSMA pattern (0..7)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static radio_burst_t bursts[TSUNBPHY_MAX_BURSTS];
static uint16_t num_bursts;

// test program: fixed ETSI TS 103 357-2 Annex D.3 test vector MPDU (15 bytes, "MIOTY")
static const uint8_t mpdu[] = { 0x00, 0xf0, 0x01, 0x0b, 0x8d, 0xe1, 0x4c, 0xbf,
                                0x63, 0x8d, 0xdc, 0xd3, 0xd2, 0x30, 0xae };
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

static void send_uplink(void){
    BSP_LED_On(LED_BLUE);                        // blue: transmitting
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

  // the MPDU never changes, so the frame is encoded once
  num_bursts = phy_encode(bursts, mpdu, sizeof(mpdu), APP_TSMA_PATTERN);

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

    // B1 in trigger mode: send one uplink per press (ignored in auto mode)
    uint8_t b1 = button_pressed(B1);
    if (app_mode == APP_MODE_TRIGGER && b1) {
        send_uplink();
    }
    if (app_mode == APP_MODE_AUTO && (HAL_GetTick() - last_tx) >= APP_AUTO_INTERVAL_MS) {
        last_tx = HAL_GetTick();
        send_uplink();
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
