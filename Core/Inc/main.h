/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Bsp_Led_Pin GPIO_PIN_13
#define Bsp_Led_GPIO_Port GPIOC
#define Body_PA1_Pin GPIO_PIN_1
#define Body_PA1_GPIO_Port GPIOA
#define DHT11_PA11_Pin GPIO_PIN_11
#define DHT11_PA11_GPIO_Port GPIOA
#define SCL_PB8_Pin GPIO_PIN_8
#define SCL_PB8_GPIO_Port GPIOB
#define SDA_PB9_Pin GPIO_PIN_9
#define SDA_PB9_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
/* USER CODE BEGIN Prototypes */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
