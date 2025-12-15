#include "led.h"
#include "main.h"
bool led_status = false;
bool fire_status = false;
bool body_status = false;
void Led_Set(_Bool status)
{
    led_status = status;
    if (led_status)
        Bsp_LedON();
    else
        Bsp_LedOFF();
}
void Get_Fire_State(void)
{
    fire_status = (HAL_GPIO_ReadPin(Fire_GPIO_Port, Fire_Pin) == GPIO_PIN_RESET) ? 1 : 0;
    if (HAL_GPIO_ReadPin(Fire_GPIO_Port, Fire_Pin) == GPIO_PIN_RESET) // 低电平有效
    {
        printf("有火\r\n");
    }
    else
    {
        printf("无火\r\r");
    }
}

void Body_State(void)
{
    body_status = (HAL_GPIO_ReadPin(Body_PA1_GPIO_Port, Body_PA1_Pin) == GPIO_PIN_RESET) ? 1 : 0;
    if (body_status) // 低电平有效
    {
        printf("有人\r\n");
    }
    else
    {
        printf("无有人\r\r");
    }
}
