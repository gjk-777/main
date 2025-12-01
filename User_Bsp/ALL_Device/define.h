#ifndef __DEFINE_H
#define __DEFINE_H
#include "main.h"

#define Bsp_LedON() HAL_GPIO_WritePin(Bsp_Led_GPIO_Port, Bsp_Led_Pin, GPIO_PIN_RESET)
#define Bsp_LedOFF() HAL_GPIO_WritePin(Bsp_Led_GPIO_Port, Bsp_Led_Pin, GPIO_PIN_SET)
#define Bsp_LedToggle() HAL_GPIO_TogglePin(Bsp_Led_GPIO_Port, Bsp_Led_Pin)

#endif
