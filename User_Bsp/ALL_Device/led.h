#ifndef __DEFINE_H
#define __DEFINE_H
#include "main.h"
#include "stdbool.h"
extern bool led_status;
extern bool fire_status;
extern bool body_status;
extern bool cooking_status;
extern bool fan_status;
extern bool fan_manual_mode; /* 风扇手动模式标志：APP手动控制时为true */
extern uint8_t window_angle_status;
extern uint8_t famen_angle_status;

/*
 * 引脚功能映射:
 *   PB14 (Fan_Pin_Pin)      -> 照明灯 (LED)
 *   PB15 (ZhaoMing_LED_Pin) -> 风扇   (Fan)
 *   PB13 (LED_Beep_Pin)     -> 报警灯 (Alarm LED, 由led_manager通道1管理)
 *
 * led_manager通道分配:
 *   通道0: 照明灯 (PB14)
 *   通道1: 报警灯 (PB13) - 支持闪烁
 */

/* 照明灯 PB14 */
#define ZhaoMingLED_ON() HAL_GPIO_WritePin(Fan_Pin_GPIO_Port, Fan_Pin_Pin, GPIO_PIN_RESET)
#define ZhaoMingLED_OFF() HAL_GPIO_WritePin(Fan_Pin_GPIO_Port, Fan_Pin_Pin, GPIO_PIN_SET)

/* 风扇 PB15 */
#define FanMotor_ON() HAL_GPIO_WritePin(ZhaoMing_LED_GPIO_Port, ZhaoMing_LED_Pin, GPIO_PIN_RESET)
#define FanMotor_OFF() HAL_GPIO_WritePin(ZhaoMing_LED_GPIO_Port, ZhaoMing_LED_Pin, GPIO_PIN_SET)

/* 报警灯 PB13 */
#define AlarmLedOFF() HAL_GPIO_WritePin(LED_Beep_GPIO_Port, LED_Beep_Pin, GPIO_PIN_RESET)
#define AlarmLedON() HAL_GPIO_WritePin(LED_Beep_GPIO_Port, LED_Beep_Pin, GPIO_PIN_SET)

void Led_Set(_Bool status);
void Fan_Set(_Bool status);
void Get_Fire_State(void);
void Body_State(void);

void ButtonHandler(void);

void LED_Manager_Init(void);
void LED_Manager_Usage(void);
void Servo_angle(uint8_t angle);
void Servo_angle_ex(uint8_t angle, bool save);
void Famen_angle(uint8_t angle);
void Famen_angle_ex(uint8_t angle, bool save);
#endif
