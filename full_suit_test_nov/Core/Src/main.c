/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "i2c.h"
#include "usart.h"
#include "gpio.h"
#include "ina228_driver_logging.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */

/* ================= USER DEFINES ================= */
#define RX_BUF_SIZE        64
#define UART_TIMEOUT_MS    1000U
#define SHUNT1_OHMS        0.003f  /* VERIFY against sensor 1 physical shunt */
#define SHUNT2_OHMS        0.005f  /* VERIFY against sensor 2 physical shunt */

/* Forward declarations required before main() in C. */
static void UART_ReceiveLine(void);
static int Parse_Command(void);
static void Acquire_And_Stream(void);

static char rxBuf[RX_BUF_SIZE];
static int rxIndex;
static uint32_t sampling_rate = 0;
static uint32_t total_time = 0;
static uint32_t num_samples = 0;
static uint8_t sensor_count = 1;

static void Send_Text(const char *message)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)message,
                    (uint16_t)strlen(message), UART_TIMEOUT_MS);
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();


  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2,
                    GPIO_PIN_RESET);

  /* Initialize sensor 1. Sensor 2 is initialized only when requested. */
  if (INA228_Init(INA228_ADDR1, SHUNT1_OHMS) != HAL_OK)
    Send_Text("WARN_SENSOR1_INIT\n");

  while (1)
  {
    UART_ReceiveLine();
    if (!Parse_Command())
    {
      Send_Text("ERR_COMMAND\n");
      continue;
    }

    if (sensor_count == 2 &&
        INA228_Init(INA228_ADDR2, SHUNT2_OHMS) != HAL_OK)
    {
      Send_Text("ERR_SENSOR2_INIT\n");
      continue;
    }

    uint8_t healthy1 = 0, healthy2 = 0;
    if (INA228_CheckHealth(INA228_ADDR1, &healthy1) != HAL_OK || !healthy1)
    {
      Send_Text("ERR_SENSOR1_HEALTH\n");
      continue;
    }
    if (sensor_count == 2 &&
        (INA228_CheckHealth(INA228_ADDR2, &healthy2) != HAL_OK || !healthy2))
    {
      Send_Text("ERR_SENSOR2_HEALTH\n");
      continue;
    }

    Send_Text("OK\n");
    Acquire_And_Stream();
  }
}

/* Wait for a full UART command line. */
static void UART_ReceiveLine(void)
{
  rxIndex = 0;
  memset(rxBuf, 0, sizeof(rxBuf));
  while (1)
  {
    uint8_t byte;
    if (HAL_UART_Receive(&huart2, &byte, 1, HAL_MAX_DELAY) != HAL_OK)
      continue;
    if (byte == '\n')
      break;
    if (byte == '\r')
      continue;
    if (rxIndex < RX_BUF_SIZE - 1)
      rxBuf[rxIndex++] = (char)byte;
  }
}

/* START,<rate_hz>,<duration_s>,<sensors> (last field optional: 1) */
static int Parse_Command(void)
{
    unsigned long rate = 0;
    unsigned long duration = 0;
    unsigned long sensors = 1;
    char extra;

    int matched = sscanf(
        rxBuf,
        "START,%lu,%lu,%lu%c",
        &rate,
        &duration,
        &sensors,
        &extra
    );
    char debug[128];

    int len = snprintf(debug, sizeof(debug),
        "DEBUG: matched=%d rate=%lu duration=%lu sensors=%lu\r\n",
        matched, rate, duration, sensors);

    HAL_UART_Transmit(&huart2,
                      (uint8_t *)debug,
                      len,
                      HAL_MAX_DELAY);

    if (matched != 3)
        return 0;

    if (rate < 1 || rate > 200 ||
        duration < 1 || duration > 86400 ||
        (sensors != 1 && sensors != 2))
        return 0;

    if ((uint64_t)rate * duration > UINT32_MAX)
        return 0;

    sampling_rate = (uint32_t)rate;
    total_time = (uint32_t)duration;
    sensor_count = (uint8_t)sensors;
    num_samples = sampling_rate * total_time;

    return 1;
}

/* No large SRAM buffers: one measurement row is streamed at a time. */
static void Acquire_And_Stream(void)
{
  char line[160];
  uint32_t start_ms = HAL_GetTick();
  uint32_t overruns = 0;
  uint32_t sent = 0;

  for (uint32_t i = 0; i < num_samples; ++i)
  {
    /* Integer-ratio schedule avoids cumulative drift from HAL_Delay(). */
    uint32_t due_ms = start_ms +
                      (uint32_t)(((uint64_t)i * 1000U) / sampling_rate);
    while ((int32_t)(HAL_GetTick() - due_ms) < 0) { }
    if (i > 0 && (int32_t)(HAL_GetTick() - due_ms) > 0)
      ++overruns;

    float v1 = 0, c1 = 0, p1 = 0;
    float v2 = 0, c2 = 0, p2 = 0;
    if (INA228_ReadVoltage(INA228_ADDR1, &v1) != HAL_OK ||
        INA228_ReadCurrent(INA228_ADDR1, &c1) != HAL_OK ||
        INA228_ReadPower(INA228_ADDR1, &p1) != HAL_OK)
    {
      Send_Text("ERR_SENSOR1_READ\n");
      return;
    }
    if (sensor_count == 2)
    {
      if (INA228_ReadVoltage(INA228_ADDR2, &v2) != HAL_OK ||
          INA228_ReadCurrent(INA228_ADDR2, &c2) != HAL_OK ||
          INA228_ReadPower(INA228_ADDR2, &p2) != HAL_OK)
      {
        Send_Text("ERR_SENSOR2_READ\n");
        return;
      }
    }

    uint32_t t_ms = HAL_GetTick() - start_ms;
    int len;
    if (sensor_count == 1)
      len = snprintf(line, sizeof(line), "%lu,%.6f,%.6f,%.6f\n",
                     (unsigned long)t_ms, v1, c1, p1);
    else
      len = snprintf(line, sizeof(line),
                     "%lu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
                     (unsigned long)t_ms, v1, c1, p1, v2, c2, p2);
    if (len <= 0 || len >= (int)sizeof(line))
    {
      Send_Text("ERR_FORMAT\n");
      return;
    }
    if (HAL_UART_Transmit(&huart2, (uint8_t *)line, (uint16_t)len,
                          UART_TIMEOUT_MS) != HAL_OK)
      return;
    ++sent;
  }

  snprintf(line, sizeof(line), "DONE,%lu,%lu\n",
           (unsigned long)sent, (unsigned long)overruns);
  Send_Text(line);
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
RCC_OscInitTypeDef RCC_OscInitStruct = {0};
RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

__HAL_RCC_PWR_CLK_ENABLE();
__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
RCC_OscInitStruct.HSIState = RCC_HSI_ON;
RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
HAL_RCC_OscConfig(&RCC_OscInitStruct);

RCC_ClkInitStruct.ClockType =
	RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
	RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;

RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */

void Error_Handler(void)
{
	;
}
