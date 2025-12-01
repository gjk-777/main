#include "oled_display.h"
#include "OLED.h"
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

    ReadTime(TimeArr);
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
void Data_Show(uint8_t *temp, uint8_t *humi)
{
    OLED_ShowString(TEMP_HUMI_LOGO_POSITION_X, TEMP_HUMI_LOGO_POSITION_Y, " T:00C   H:000% ", OLED_8X16);

    OLED_ShowNum(TEMP_NUM_POSITION_X, TEMP_NUM_POSITION_Y, *humi, TEMP_NUM_LEN, OLED_8X16);
    OLED_ShowNum(HUMI_NUM_POSITION_X, HUMI_NUM_POSITION_Y, *temp, HUMI_NUM_LEN, OLED_8X16);
}
