#ifndef __OLED_DISPLAY_H__
#define __OLED_DISPLAY_H__

#include "stdint.h"
#include "stdbool.h"

/* 时间显示 */
#define POSITION_TIME_X 32
#define POSITION_TIME_Y 0

#define POSITION_HOUR_X 0
#define POSITION_HOUR_Y 0

#define POSITION_MIN_X 56
#define POSITION_MIN_Y 0

#define POSITION_SEC_X 80
#define POSITION_SEC_Y 0

#define POSITION_TIME_LEN 2

/* 温湿度 */
#define TEMP_HUMI_LOGO_POSITION_X 0
#define TEMP_HUMI_LOGO_POSITION_Y 16

#define TEMP_NUM_POSITION_X 24
#define TEMP_NUM_POSITION_Y 16
#define TEMP_NUM_LEN 2

#define HUMI_NUM_POSITION_X 88
#define HUMI_NUM_POSITION_Y 16
#define HUMI_NUM_LEN 3

/* 界面切换模式 */
#define DISPLAY_MODE_SENSOR 0  /* 传感器数据界面 */
#define DISPLAY_MODE_CONTROL 1 /* 控制设备状态界面 */

void TimeDisplay(void);
void Data_Show(uint8_t *temp, uint8_t *humi, float *smoke, float *co);
void Control_Show(void);
void OLED_Display_Switch(uint8_t mode, uint8_t *temp, uint8_t *humi, float *smoke, float *co);
void OLED_Smooth_Transition(uint8_t from_mode, uint8_t to_mode, uint8_t *temp, uint8_t *humi, float *smoke, float *co);
void ESP_link_imag();
#endif
