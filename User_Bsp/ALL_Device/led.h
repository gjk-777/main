#ifndef __DEFINE_H
#define __DEFINE_H
#include "main.h"
#include "stdbool.h"
extern bool led_status;
extern bool fire_status;
extern bool body_status;
extern bool cooking_status;
extern bool fan_status;
extern uint8_t window_angle_status;
void Led_Set(_Bool status);
// #define Led_SetStatus(X) HAL_GPIO_WritePin(Bsp_Led_GPIO_Port, Bsp_Led_Pin, (!X))

#define Bsp_LedON() HAL_GPIO_WritePin(Bsp_Led_GPIO_Port, Bsp_Led_Pin, GPIO_PIN_RESET)
#define Bsp_LedOFF() HAL_GPIO_WritePin(Bsp_Led_GPIO_Port, Bsp_Led_Pin, GPIO_PIN_SET)
#define Bsp_LedToggle() HAL_GPIO_TogglePin(Bsp_Led_GPIO_Port, Bsp_Led_Pin)

#define Led_BeepON() HAL_GPIO_WritePin(Bsp_Led_GPIO_Port, LED_Beep_Pin, GPIO_PIN_RESET)
#define Led_BeepOFF() HAL_GPIO_WritePin(Bsp_Led_GPIO_Port, LED_Beep_Pin, GPIO_PIN_SET)
#define Led_BeepToggle() HAL_GPIO_TogglePin(Bsp_Led_GPIO_Port, LED_Beep_Pin)
void Get_Fire_State(void);
void Body_State(void);

void ButtonHandler(void);

// 初始化LED管理器示例
void LED_Manager_Init(void);
// 使用LED管理器控制LED示例
void LED_Manager_Usage(void);
void Servo_angle(uint8_t angle);
void Famen_angle(uint8_t angle);
#endif
