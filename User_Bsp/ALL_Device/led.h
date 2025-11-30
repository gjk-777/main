#ifndef __LED_H__
#define __LED_H__

#include "main.h"

#define Bsp_LedON HAL_GPIO_WritePin(Bsp_Led_GPIO_Port, Bsp_Led_Pin, GPIO_PIN_RESET)
#define Bsp_LedOFF HAL_GPIO_WritePin(Bsp_Led_GPIO_Port, Bsp_Led_Pin, GPIO_PIN_SET)
#define Bsp_LedToggle HAL_GPIO_TogglePin(Bsp_Led_GPIO_Port, Bsp_Led_Pin)

#endif
