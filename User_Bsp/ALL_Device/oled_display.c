#include "oled_display.h"
#include "OLED.h"
#include "OLED_Data.h"
#include "stm32f1xx_hal.h"
#include "rtc.h"
#include "led.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include <stdio.h>
#include <string.h>

extern uint8_t OLED_DisplayBuf[8][128];
extern EventGroupHandle_t xAlarmEvent;

/* 事件位定义（与My_Task.c保持一致） */
#define EVENT_TEMP_BITS (0x01 << 0)
#define EVENT_MQ2_BITS (0x01 << 1)
#define EVENT_CO_MQ7_BITS (0x01 << 2)
#define EVENT_FIRE_BITS (0x01 << 3)
#define EVENT_BODY_BITS (0x01 << 4)

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
    OLED_ShowString(40, 16, "        ", OLED_8X16); /* 先清空旧数据区域，防残留 */
    sprintf(buf_Data, "%.2f%%", *smoke);
    OLED_ShowString(40, 16, buf_Data, OLED_8X16);

    OLED_ShowChinese(0, 32, "甲烷");
    OLED_ShowChar(32, 32, ':', OLED_8X16);
    OLED_ShowString(40, 32, "          ", OLED_8X16); /* 先清空旧数据区域，防残留 */
    sprintf(buf_Data, "%.1fppm", *co);
    OLED_ShowString(40, 32, buf_Data, OLED_8X16);

    /* 第四行：火灾状态和人体状态显示 */
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

    /* 分隔符 */
    OLED_ShowChar(56, 48, '|', OLED_8X16);

    /* 人体状态显示（在火灾后面，固定显示有） */
    OLED_ShowChinese(64, 48, "人体");
    OLED_ShowChar(96, 48, ':', OLED_8X16);
    OLED_ShowChinese(104, 48, "有");
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

/**
 * @brief 控制设备状态显示界面（静态绘制，不含动画）
 * @note 显示照明、风扇、窗户角度、阀门角度、报警状态
 * 布局（128x64, 16px行网格）：
 *   Row0 (Y=0):  照明:开/关  |  风扇:开/关
 *   ─── 分隔线 (Y=15) ───
 *   Row1 (Y=16): 窗户:xxx°  [====    ] 进度条
 *   ─── 分隔线 (Y=31) ───
 *   Row2 (Y=32): 阀门:xxx°  [====    ] 进度条
 *   ─── 分隔线 (Y=47) ───
 *   Row3 (Y=48): 当前状态:正常/报警...
 */
void Control_Show(void)
{
    static char buf[20];
    EventBits_t alarm_bits = 0;

    /* 读取当前报警事件位 */
    if (xAlarmEvent != NULL)
    {
        alarm_bits = xEventGroupGetBits(xAlarmEvent);
    }

    /* ===== Row0 (Y=0): 照明 + 风扇（紧凑布局，各占64px） ===== */
    OLED_ShowChinese(0, 0, "照明：");
    if (led_status)
    {
        OLED_ShowChinese(48, 0, "开");
    }
    else
    {
        OLED_ShowChinese(48, 0, "关");
    }

    /* 中间竖线分隔（1px） */
    OLED_DrawLine(64, 2, 64, 13);

    OLED_ShowChinese(66, 0, "风扇：");
    if (fan_status)
    {
        OLED_ShowChinese(114, 0, "开");
    }
    else
    {
        OLED_ShowChinese(114, 0, "关");
    }

    /* 水平分隔线 */
    OLED_DrawLine(0, 15, 127, 15);

    /* ===== Row1 (Y=16): 窗户角度 ===== */
    OLED_ShowChinese(0, 16, "窗户");
    OLED_ShowChar(16, 16, ':', OLED_8X16);
    OLED_ShowString(24, 16, "    ", OLED_8X16); /* 清空旧数据 */
    sprintf(buf, "%d", window_angle_status);
    OLED_ShowString(24, 16, buf, OLED_8X16);
    /* 度数符号°（小圆圈模拟） */
    OLED_DrawCircle(24 + strlen(buf) * 8 + 2, 24, 2, OLED_UNFILLED);

    /* 角度进度条：背景框 + 填充 */
    OLED_DrawRectangle(56, 21, 64, 6, OLED_UNFILLED);
    {
        uint8_t bar_len = (uint8_t)((window_angle_status * 64) / 90);
        if (bar_len > 0)
        {
            OLED_DrawRectangle(56, 21, bar_len, 6, OLED_FILLED);
        }
    }

    /* 水平分隔线 */
    OLED_DrawLine(0, 31, 127, 31);

    /* ===== Row2 (Y=32): 阀门角度 ===== */
    OLED_ShowChinese(0, 32, "阀门");
    OLED_ShowChar(16, 32, ':', OLED_8X16);
    OLED_ShowString(24, 32, "    ", OLED_8X16); /* 清空旧数据 */
    sprintf(buf, "%d", famen_angle_status);
    OLED_ShowString(24, 32, buf, OLED_8X16);
    /* 度数符号°（小圆圈模拟） */
    OLED_DrawCircle(24 + strlen(buf) * 8 + 2, 40, 2, OLED_UNFILLED);

    /* 角度进度条：背景框 + 填充 */
    OLED_DrawRectangle(56, 37, 64, 6, OLED_UNFILLED);
    {
        uint8_t bar_len = (uint8_t)((famen_angle_status * 64) / 90);
        if (bar_len > 0)
        {
            OLED_DrawRectangle(56, 37, bar_len, 6, OLED_FILLED);
        }
    }

    /* 水平分隔线（状态区上方） */
    OLED_DrawLine(0, 47, 127, 47);

    /* ===== Row3 (Y=48): 状态 ===== */
    OLED_ShowChinese(0, 48, "状态：");

    /* 清空状态信息显示区域，防止残留 */
    OLED_ShowString(48, 48, "            ", OLED_8X16);

    /* 根据事件位判断报警类型（优先级：火灾 > 燃气 > 烟雾 > 温度） */
    if (alarm_bits & EVENT_FIRE_BITS)
    {
        OLED_ShowChinese(48, 48, "火警");
    }
    else if (alarm_bits & EVENT_CO_MQ7_BITS)
    {
        OLED_ShowChinese(48, 48, "燃气超标");
    }
    else if (alarm_bits & EVENT_MQ2_BITS)
    {
        OLED_ShowChinese(48, 48, "浓度过高");
    }
    else if (alarm_bits & EVENT_TEMP_BITS)
    {
        OLED_ShowChinese(48, 48, "温度异常");
    }
    else
    {
        OLED_ShowChinese(48, 48, "正常");
    }
}

/**
 * @brief 平滑过渡动画效果（丝滑版）
 * @param from_mode 当前界面模式
 * @param to_mode 目标界面模式
 * @param temp 温度指针
 * @param humi 湿度指针
 * @param smoke 烟雾浓度指针
 * @param co 甲烷浓度指针
 * @note 淡出：匀速清除；淡入：递增延时，产生减速"瀑布落定"效果
 */
void OLED_Smooth_Transition(uint8_t from_mode, uint8_t to_mode, uint8_t *temp, uint8_t *humi, float *smoke, float *co)
{
    /* 淡出效果：匀速从上到下逐行清除 */
    for (uint8_t i = 0; i < 8; i++)
    {
        memset(OLED_DisplayBuf[i], 0, 128);
        OLED_UpdateArea(0, i * 8, 128, 8);
        vTaskDelay(pdMS_TO_TICKS(15)); /* 15ms/page，共120ms */
    }

    /* 清空显示缓冲区 */
    OLED_Clear();

    /* 根据目标模式绘制新界面 */
    if (to_mode == DISPLAY_MODE_SENSOR)
    {
        Data_Show(temp, humi, smoke, co);
    }
    else if (to_mode == DISPLAY_MODE_CONTROL)
    {
        Control_Show();
    }

    /* 淡入效果：递增延时，产生丝滑"减速落定"动画
     * 延时序列: 8,10,12,15,18,22,26,30 ms
     * 前几行快速闪现，后几行逐渐减速，像水滴落下自然停住 */
    static const uint8_t fade_in_delay[8] = {8, 10, 12, 15, 18, 22, 26, 30};
    for (uint8_t i = 0; i < 8; i++)
    {
        OLED_UpdateArea(0, i * 8, 128, 8);
        vTaskDelay(pdMS_TO_TICKS(fade_in_delay[i]));
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

    /* 根据模式绘制对应界面 */
    if (mode == DISPLAY_MODE_SENSOR)
    {
        Data_Show(temp, humi, smoke, co);
    }
    else if (mode == DISPLAY_MODE_CONTROL)
    {
        Control_Show();
    }
    OLED_Update();
}
