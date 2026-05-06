/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "dma.h"
#include "fdcan.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SAMPLES 25
#define MODULES_NUM 3
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern DMA_HandleTypeDef hdma_tim1_ch1;
volatile int gpio_transfer_done=0;
volatile uint8_t data_ready[MODULES_NUM] = {0};
volatile uint32_t gpio_buffer[SAMPLES] = {0};
int32_t reading[MODULES_NUM] = {0};
volatile int measuring_request = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void get_reading(int32_t *read, uint32_t *gpio_buff){
  for(int i=0; i<MODULES_NUM; i++)
    read[i] = 0;
  for(int i=0; i<SAMPLES - 1; i++){
    for(int j=0; j<MODULES_NUM; j++){
      uint32_t one_bit = (gpio_buff[i] >> (5+j)) & 1;
      read[j] |= (one_bit << (SAMPLES - 1 - i));  //kolejnosc MSB
    }
  }
  for(int i=0; i<MODULES_NUM; i++){
    read[i] = (read[i] << 8) >> 8;  //ustawienie bitu znaku
  }

}

void DMA_TransferCompleteCallback(DMA_HandleTypeDef *hdma){
  gpio_transfer_done = 1;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  switch(GPIO_Pin){
    case DATA1_Pin:
      data_ready[0] = 1;
      break;
    case DATA2_Pin:
      data_ready[1] = 1;
      break;
    case DATA3_Pin:
      data_ready[2] = 1;
      break;
  }
  if(data_ready[0] && data_ready[1] && data_ready[2]){
    measuring_request = 1;
  }
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

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_FDCAN1_Init();
  /* USER CODE BEGIN 2 */
  hdma_tim1_ch1.XferCpltCallback = DMA_TransferCompleteCallback;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    if(measuring_request == 1){
      HAL_NVIC_DisableIRQ(EXTI4_15_IRQn); //wylaczenie przerwan na EXTI na czas przetwarzania odczytu
      HAL_DMA_Start_IT(&hdma_tim1_ch1, (uint32_t)&GPIOA->IDR, (uint32_t)gpio_buffer, SAMPLES);
      __HAL_TIM_ENABLE_DMA(&htim1, TIM_DMA_CC1);
      HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1); //timer w one pulse mode, RCR=25-1
      gpio_transfer_done=0;
      data_ready[0] = 0;
      data_ready[1] = 0;
      data_ready[2] = 0;
    }
    if(gpio_transfer_done == 1){
      HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);  //wlaczenie przerwan EXTI po pomiarze
      gpio_transfer_done=0;
      get_reading(reading, gpio_buffer);
    }
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
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
  __disable_irq();
  while (1)
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
