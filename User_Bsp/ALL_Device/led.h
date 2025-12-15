#ifndef __DEFINE_H
#define __DEFINE_H
#include "main.h"
#include "stdbool.h"
extern bool led_status;
extern bool fire_status;
extern bool body_status;
void Led_Set(_Bool status);
// #define Led_SetStatus(X) HAL_GPIO_WritePin(Bsp_Led_GPIO_Port, Bsp_Led_Pin, (!X))

#define Bsp_LedON() HAL_GPIO_WritePin(Bsp_Led_GPIO_Port, Bsp_Led_Pin, GPIO_PIN_RESET)
#define Bsp_LedOFF() HAL_GPIO_WritePin(Bsp_Led_GPIO_Port, Bsp_Led_Pin, GPIO_PIN_SET)
#define Bsp_LedToggle() HAL_GPIO_TogglePin(Bsp_Led_GPIO_Port, Bsp_Led_Pin)
void Get_Fire_State(void);
void Body_State(void);

#endif
