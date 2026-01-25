#include "oled_display.h"
#include "OLED.h"
#include "OLED_Data.h"
#include "stm32f1xx_hal.h"

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

/**
 * @description: 显示温湿度、光照度数据
 * @param {uint8_t} *temp
 * @param {uint8_t} *humi
 * @param {uint8_t} *LightIntensity
 * @return {*}
 * @Date: 2024-10-24 10:52:08
 * @Author: Jchen
 */
// void Data_Show(uint8_t *temp, uint8_t *humi)
//{
//     // 清除原来的显示
//     OLED_ShowString(TEMP_HUMI_LOGO_POSITION_X, TEMP_HUMI_LOGO_POSITION_Y, "                ", OLED_8X16);

//    // 显示中文字符
//    OLED_ShowChinese(TEMP_HUMI_LOGO_POSITION_X + 0, TEMP_HUMI_LOGO_POSITION_Y, "纪凯");
//    OLED_ShowString(TEMP_HUMI_LOGO_POSITION_X + 32, TEMP_HUMI_LOGO_POSITION_Y, ":00C  ", OLED_8X16);

//    OLED_ShowChinese(TEMP_HUMI_LOGO_POSITION_X + 64, TEMP_HUMI_LOGO_POSITION_Y, "明杰");
//    OLED_ShowString(TEMP_HUMI_LOGO_POSITION_X + 96, TEMP_HUMI_LOGO_POSITION_Y, ":00%  ", OLED_8X16);

//    // 显示数字，调整位置以匹配中文字符后的显示位置
//    OLED_ShowNum(TEMP_HUMI_LOGO_POSITION_X + 40, TEMP_HUMI_LOGO_POSITION_Y, *humi, TEMP_NUM_LEN, OLED_8X16);
//    OLED_ShowNum(TEMP_HUMI_LOGO_POSITION_X + 104, TEMP_HUMI_LOGO_POSITION_Y, *temp, HUMI_NUM_LEN, OLED_8X16);
//}

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

void Data_Show(uint8_t *temp, uint8_t *humi, float *smoke, float *co)
{
    // Row 1: Temperature and Humidity
    OLED_ShowChinese(0, 0, "温度");
    OLED_ShowChar(32, 0, ':', OLED_8X16);
    OLED_ShowNum(40, 0, *humi, 2, OLED_8X16);
    OLED_ShowChar(56, 0, 'C', OLED_8X16);

    OLED_ShowChinese(64, 0, "湿度");
    OLED_ShowChar(96, 0, ':', OLED_8X16);
    OLED_ShowNum(104, 0, *temp, 2, OLED_8X16);
    OLED_ShowChar(120, 0, '%', OLED_8X16);

    // Row 2: Smoke (MQ2)
    OLED_ShowChinese(0, 16, "烟雾");
    OLED_ShowChar(32, 16, ':', OLED_8X16);
    // Display float: 3 Int + 1 Dot + 1 Frac = 5 chars
    OLED_ShowFloatNum(40, 16, *smoke, 3, 1, OLED_8X16);
    OLED_ShowString(80, 16, "ppm", OLED_8X16);

    // Row 3: CO (MQ7)
    OLED_ShowString(0, 32, "CO", OLED_8X16);
    OLED_ShowChar(16, 32, ':', OLED_8X16);
    // Display float
    OLED_ShowFloatNum(24, 32, *co, 3, 1, OLED_8X16);
    OLED_ShowString(64, 32, "ppm", OLED_8X16);

    // Row 4: Clear previous content
    OLED_ShowString(0, 48, "                ", OLED_8X16);
}


