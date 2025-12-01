#include "My_Task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "tim.h"

#include "OLED.h"
#include "define.h"
#include "oled_display.h"
#include "buzzer.h"
#include "dht11.h"
#include "usart.h"
#include "string.h"
#include "stdio.h"
static uint8_t Body_val;
static uint8_t Body_val_last;
extern uint8_t Uart1RxBuffer[100];
TaskHandle_t xLedTaskHandle;
TaskHandle_t xHomeTaskHandle;

static void Name_Show()
{

    OLED_ShowChinese(0, 0, "郭纪凯杜明杰王鑫");
    OLED_ShowChinese(0, 16, "宇");
    OLED_ShowChinese(0, 32, "宇");
    OLED_ShowChinese(0, 48, "宇");
    OLED_ShowChinese(115, 48, "郭");
}

void My_Led_Task(void *pvParameters)
{
    Uart_printf(USART_DEBUG, "Connect MQTTs Server...\r\n");
    while (1)
    {
        Bsp_LedToggle();
        // printf("led亮\r\n");
        //  PassiveBuzzer_Test();
        vTaskDelay(1000);
    }
}
void Home_Task(void *pvParameters)
{
    uint8_t count = 5;
    uint8_t humi = 0;
    uint8_t temp = 0;
    while (1)
    {
        OLED_Clear();
        //        TimeDisplay();
        //        /* 每5秒更新一次温湿度数据 */
        //        if (count == 5)
        //        {
        //            count = 0;
        //            DHT11Senser_Read(&humi, &temp);
        //        }
        //        Data_Show(&temp, &humi);
        // Name_Show();
        Body_val = HAL_GPIO_ReadPin(Body_PA1_GPIO_Port, Body_PA1_Pin);
        //        if (Body_val != Body_val_last)
        //        {

        if (Body_val == GPIO_PIN_SET)
        {
            printf("有人\r\n");
        }
        else
        {
            printf("无人\r\n");
        }
        // Body_val_last = Body_val;
        //}

        OLED_Update();
        vTaskDelay(500);
    }
}

void My_Task_Init(void)
{
    xTaskCreate(My_Led_Task, "My_Led_Task", 128, NULL, 10, &xLedTaskHandle);
    xTaskCreate(Home_Task, "My_Home_Task", 128, NULL, 10, &xHomeTaskHandle);
}

void My_Drivers_Init(void)
{
    HAL_TIM_Base_Start_IT(&htim2);
    /* 需要在初始化时调用一次否则无法接收到内容 */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, Uart1RxBuffer, 100);
    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT); // 手动关闭DMA_IT_HT中断
    PassiveBuzzer_Init();
    OLED_Init();
    OLED_Clear();
    My_Task_Init();
}
