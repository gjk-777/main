#include "oled_display.h"
#include "OLED.h"
#include "OLED_Data.h"
#include "stm32f1xx_hal.h"
#include "rtc.h"
#include "led.h"
#include <stdio.h>
#include <string.h>

extern uint8_t OLED_DisplayBuf[8][128];
// OLED_Buf_Backup removed to save RAM

/**
 * @description: 时间显示
 * @return {*}
 * @Date: 2024-10-23 16:58:02
 * @Author: Jchen
 */
void TimeDisplay(void)
{
    uint8_t TimeArr[3];
    static uint8_t count = 1;

    /* 显示时间冒分割号 */
    if (count == 1)
    {
        OLED_ShowString(POSITION_TIME_X, POSITION_TIME_Y, "  :  :  ", OLED_8X16);
        count = 0;
    }
    else
    {
        OLED_ShowString(POSITION_TIME_X, POSITION_TIME_Y, "        ", OLED_8X16);
        count++;
    }

    ReadTime(&TimeArr);
    /* 显示时间数字 */
    OLED_ShowNum(POSITION_HOUR_X, POSITION_HOUR_Y, TimeArr[0], POSITION_TIME_LEN, OLED_8X16);
    OLED_ShowNum(POSITION_MIN_X, POSITION_MIN_Y, TimeArr[1], POSITION_TIME_LEN, OLED_8X16);
    OLED_ShowNum(POSITION_SEC_X, POSITION_SEC_Y, TimeArr[2], POSITION_TIME_LEN, OLED_8X16);
}

static char date_str[20];
static char time_str[20];
void View_Time(void)
{
    MyRTC_ReadTime();

    // 显示 "当前日期" (Centered: 32px)
    OLED_ShowChinese(32, 0, "当前日期");

    // 显示日期 YYYY-MM-DD (Centered: 24px)
    sprintf(date_str, "%04d-%02d-%02d", MyRTC_Time[0], MyRTC_Time[1], MyRTC_Time[2]);
    OLED_ShowString(24, 16, date_str, OLED_8X16);

    // 显示 "当前时间" (Centered: 32px)
    OLED_ShowChinese(32, 32, "当前时间");

    // 显示时间 HH:MM:SS (Centered: 32px)
    sprintf(time_str, "%02d:%02d:%02d", MyRTC_Time[3], MyRTC_Time[4], MyRTC_Time[5]);
    OLED_ShowString(32, 48, time_str, OLED_8X16);
}

#define OPTION_TITLE_POSITION_X 30
#define OPTION_TITLE_POSITION_Y 2
#define OPTION_STATUS_POSITION_X 65
#define OPTION_STATUS_POSITION_Y 2
#define OPTION_ICON_POSITION_X 40
#define OPTION_ICON_POSITION_Y 22

void ESP_link_imag()
{
    OLED_ShowString(OPTION_TITLE_POSITION_X, OPTION_TITLE_POSITION_Y, "WIFI", OLED_8X16);
    OLED_ShowChinese(OPTION_STATUS_POSITION_X, OPTION_STATUS_POSITION_Y, "连接中");
    OLED_ShowImage(OPTION_ICON_POSITION_X, OPTION_ICON_POSITION_Y, 48, 48, gImage_new);
}

static char buf_Data[20];
void Data_Show(uint8_t *temp, uint8_t *humi, float *smoke, float *co)
{
    /* 第一行：温度和湿度 */
    OLED_ShowChinese(0, 0, "温度");
    OLED_ShowChar(32, 0, ':', OLED_8X16);
    OLED_ShowNum(40, 0, *temp, 2, OLED_8X16);
    OLED_ShowChar(56, 0, 'C', OLED_8X16);

    OLED_ShowChinese(64, 0, "湿度");
    OLED_ShowChar(96, 0, ':', OLED_8X16);
    OLED_ShowNum(104, 0, *humi, 2, OLED_8X16);
    OLED_ShowChar(120, 0, '%', OLED_8X16);

    /* 第二行：烟雾浓度和CO浓度 */
    OLED_ShowChinese(0, 16, "烟雾");
    OLED_ShowChar(32, 16, ':', OLED_8X16);
    sprintf(buf_Data, "%.2f%%", *smoke);
    OLED_ShowString(40, 16, buf_Data, OLED_8X16);

    OLED_ShowChinese(0, 32, "甲烷");
    OLED_ShowChar(32, 32, ':', OLED_8X16);
    sprintf(buf_Data, "%.1fppm", *co);
    OLED_ShowString(40, 32, buf_Data, OLED_8X16);

    /* 第三行：火灾状态显示 */
    OLED_ShowChinese(0, 48, "火灾");
    OLED_ShowChar(32, 48, ':', OLED_8X16);
    if (fire_status)
    {
        OLED_ShowChinese(40, 48, "有");
    }
    else
    {
        OLED_ShowChinese(40, 48, "无");
    }
}

static void OLED_LowRAM_Transition(uint8_t mode, uint8_t *temp, uint8_t *humi, float *smoke, float *co)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        memset(OLED_DisplayBuf[i], 0, 128);
        OLED_UpdateArea(0, i * 8, 128, 8);
        vTaskDelay(20);
    }

    OLED_Clear();

    if (mode == 0) // 显示数据模式
    {
        Data_Show(temp, humi, smoke, co);
    }
    else // 显示时间模式
    {
        View_Time();
    }

    for (uint8_t i = 0; i < 8; i++)
    {
        OLED_UpdateArea(0, i * 8, 128, 8);
        vTaskDelay(20);
    }
}

void OLED_Display_Switch(uint8_t mode, uint8_t *temp, uint8_t *humi, float *smoke, float *co)
{
    static uint8_t last_mode = 255;

    if (mode != last_mode)
    {
        /* 界面切换过渡效果 */
        for (uint8_t i = 0; i < 8; i++)
        {
            memset(OLED_DisplayBuf[i], 0, 128);
            OLED_UpdateArea(0, i * 8, 128, 8);
            vTaskDelay(20);
        }
        OLED_Clear();
        last_mode = mode;
    }

    /* 只显示数据，不再切换时间界面 */
    Data_Show(temp, humi, smoke, co);
    OLED_Update();
}
