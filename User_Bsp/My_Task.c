#include "My_Task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "tim.h"
#include "cmsis_os.h"

#include "OLED.h"
#include "led.h"
#include "led_manager.h"
#include "oled_display.h"
#include "buzzer.h"
extern bool fire_status;
extern bool cooking_status;
#include "dht11.h"
#include "usart.h"
#include "string.h"
#include "stdio.h"
#include "esp8266.h"
#include "onenet.h"
#include "adc.h"
#include "rtc.h"

#include "queue.h"  //队列
#include "timers.h" //软件定时器
#include "event_groups.h"
#include "FreeRTOSConfig.h"
#include "TaskStackTracker.h"
#define ESP8266_ONENET_INFO "AT+CIPSTART=\"TCP\",\"mqtts.heclouds.com\",1883\r\n"

static uint8_t Body_val;
static uint8_t Body_val_last;
extern uint8_t Uart1RxBuffer[256];
extern uint8_t chuan;
uint8_t humi = 0;
uint8_t temp = 0;

float ppm = 0;
uint8_t count = 0;
float adc_mq2 = 0;
float adc_CO_MQ7 = 0;
bool fire_value;
unsigned char *dataPtr = NULL;
extern DMA_HandleTypeDef hdma_usart1_rx;
TaskHandle_t xLedTaskHandle;
TaskHandle_t xHomeTaskHandle;
TaskHandle_t xEspLinkTaskHandle;
TaskHandle_t xSensorTaskHandle;
TaskHandle_t xKeyGetHandle_t;
TimerHandle_t xTimerHandle_Key;

TaskStatus_t xEspLink_State; // 任务状态结构体
// 事件组
EventGroupHandle_t xAlarmEvent = NULL;

#define EVENT_TEMP_BITS (0x01 << 0)
#define EVENT_MQ2_BITS (0x01 << 1)
#define EVENT_CO_MQ7_BITS (0x01 << 2)
#define EVENT_FIRE_BITS (0x01 << 3)
#define EVENT_ALL_BITS (EVENT_TEMP_BITS | EVENT_MQ2_BITS | EVENT_CO_MQ7_BITS | EVENT_FIRE_BITS)

/* Kitchen safety application policy (application-layer only). */
#define APP_DATA_UPLOAD_PERIOD_MS 1000
#define APP_SENSOR_SCAN_PERIOD_MS 500
#define APP_TEMP_ALARM_C 32
#define APP_MQ2_COOKING_THRESHOLD 20.0f
#define APP_MQ2_NORMAL_THRESHOLD 10.0f
#define APP_CO_COOKING_THRESHOLD 50.0f
#define APP_CO_NORMAL_THRESHOLD 25.0f

static void Name_Show()
{
    OLED_ShowChinese(0, 0, "连接中");
    OLED_ShowChinese(0, 16, "宇");
    OLED_ShowChinese(0, 32, "宇");
    OLED_ShowChinese(0, 48, "宇");
    OLED_ShowChinese(115, 48, "郭");
}
static char time_str[20];
static char date_str[20];

void My_Led_Task(void *pvParameters)
{
    (void)pvParameters;
    vTaskDelay(15 * 1000);
    Servo_angle(0);
    while (1)
    {
        vTaskDelay(1000);
    }
}
void HomePage_Task(void *pvParameters)
{
    (void)pvParameters;
    static uint32_t switch_count = 0;
    static uint8_t display_mode = 1; // 0: Data, 1: Time (Start with Time)
    DHT11Senser_Read(&humi, &temp);
    while (1)
    {
        vTaskGetInfo(xEspLinkTaskHandle, &xEspLink_State, pdFALSE, eInvalid);
        if (xEspLink_State.eCurrentState == eSuspended)
        {
            // Update sensors every ~1.5 seconds (30 * 50ms)
            if (count++ >= 30)
            {
                count = 0;
                DHT11Senser_Read(&humi, &temp);
            }

            // Switch mode every ~3 seconds (60 * 50ms)
            if (++switch_count >= 60)
            {
                switch_count = 0;
                display_mode = !display_mode; // Toggle 0 <-> 1
            }
            // Handle Display Logic (Data <-> Time switch)
            OLED_Display_Switch(display_mode, &temp, &humi, &adc_mq2, &adc_CO_MQ7);
            vTaskDelay(50);
        }
        else
        {
            ESP_link_imag();
            OLED_Update();
            vTaskDelay(200);
        }
    }
}
static uint8_t k = 0;
void EspLink_Task(void *pvParameters)
{
    (void)pvParameters;
    OLED_Clear();
    ESP_link_imag();
    OLED_Update();

    // M24C02_Test();
    // PassiveBuzzer_Test();
    printf("tasktask\r\n");

    ESP8266_Init(); // 初始化ESP8266
    vTaskDelay(100);
    // Esp_Get_Time();
    //  将网络获取的时间戳写入RTC
    // MyRTC_SetTimeFromTimestamp(time);
    Uart_printf(USART_DEBUG, "网络时间已写入RTC：%lld\r\n", time);
    Uart_printf(USART_DEBUG, "Connect MQTTs Server...\r\n");

    Connect(ESP8266_ONENET_INFO, "CONNECT");
    vTaskDelay(100);
    Uart_printf(USART_DEBUG, "Connect MQTT Server Success--OKOKOKOK\r\n");
    while (OneNet_DevLink()) // 接入OneNET
    {
        if (k++ == 4)
            HAL_NVIC_SystemReset();
        vTaskDelay(100);
    }
    OneNET_Subscribe(); // 订阅主题
    Uart_printf(USART_DEBUG, "---------------------------Subscribe，Successful\r\n");

    LedManager_SetLed_OnOff(0, false);

    vTaskSuspend(NULL); // 挂起本任务
    vTaskDelay(portMAX_DELAY);
}
void Net_SendMsg_T(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        vTaskGetInfo(xEspLinkTaskHandle, &xEspLink_State, pdFALSE, eInvalid);
        if (xEspLink_State.eCurrentState == eSuspended)
        {
            OneNet_SendData(); // 发送数据
            ESP8266_Clear1_2();
        }
        vTaskDelay(pdMS_TO_TICKS(APP_DATA_UPLOAD_PERIOD_MS));
    }
}
void Net_RecvMsg_T(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        vTaskGetInfo(xEspLinkTaskHandle, &xEspLink_State, pdFALSE, eInvalid);
        if (xEspLink_State.eCurrentState == eSuspended)
        {
            dataPtr = Get_xiafa_data(2);
            if (dataPtr != NULL)
            {
                OneNet_RevPro(dataPtr);
            }
        }
        vTaskDelay(500);
    }
}
void Sensor_Task(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        vTaskGetInfo(xEspLinkTaskHandle, &xEspLink_State, pdFALSE, eInvalid);
        if (xEspLink_State.eCurrentState == eSuspended)
        {
            adc_mq2 = MQ2_GetPPM();
            adc_CO_MQ7 = CO_MQ7_GetPPM();

            xEventGroupClearBits(xAlarmEvent, EVENT_ALL_BITS);

            if (temp > APP_TEMP_ALARM_C)
            {
                printf("温度异常：%d℃\r\n", temp);
                xEventGroupSetBits(xAlarmEvent, EVENT_TEMP_BITS);
            }

            float mq2_threshold = cooking_status ? APP_MQ2_COOKING_THRESHOLD : APP_MQ2_NORMAL_THRESHOLD;
            float co_threshold = cooking_status ? APP_CO_COOKING_THRESHOLD : APP_CO_NORMAL_THRESHOLD;

            if (adc_mq2 > mq2_threshold)
            {
                printf("mq2烟雾浓度异常：%.2f%% (阈值: %.0f%%)\r\n", adc_mq2, mq2_threshold);
                xEventGroupSetBits(xAlarmEvent, EVENT_MQ2_BITS);
            }

            if (adc_CO_MQ7 > co_threshold)
            {
                printf("CO浓度异常：%.2fppm (阈值: %.0fppm)\r\n", adc_CO_MQ7, co_threshold);
                xEventGroupSetBits(xAlarmEvent, EVENT_CO_MQ7_BITS);
            }
            printf("做饭模式: %s | 温度：%d℃, CO浓度：%.2fppm, 烟雾浓度：%.2f%%\r\n",
                   cooking_status ? "开启" : "关闭", temp, adc_CO_MQ7, adc_mq2);
            Get_Fire_State();
            if (fire_status)
            {
                printf("火灾检测到！\r\n");
                xEventGroupSetBits(xAlarmEvent, EVENT_FIRE_BITS);
            }
            Body_State();
            vTaskDelay(pdMS_TO_TICKS(APP_SENSOR_SCAN_PERIOD_MS));
        }
    }
}

void Alarm_Process_Task()
{
    for (;;)
    {
        uint32_t bits = xEventGroupWaitBits(xAlarmEvent, EVENT_ALL_BITS, pdTRUE, pdFALSE, portMAX_DELAY);
        Fire_Alarm_Set(true);
        if (bits & EVENT_FIRE_BITS)
        {
            Fire_Alarm_Process();
        }
        else if (bits & EVENT_TEMP_BITS)
        {
            for (int i = 0; i < 3; i++)
            {
                PassiveBuzzer_Set_Freq_Duty(1000, 50);
                vTaskDelay(100);
                PassiveBuzzer_Set_Freq_Duty(800, 50);
                vTaskDelay(100);
            }
        }
        else if (bits & EVENT_MQ2_BITS)
        {
            for (int i = 0; i < 3; i++)
            {
                PassiveBuzzer_Set_Freq_Duty(600, 50);
                vTaskDelay(150);
                PassiveBuzzer_Set_Freq_Duty(800, 50);
                vTaskDelay(150);
            }
        }
        else if (bits & EVENT_CO_MQ7_BITS)
        {
            for (int i = 0; i < 3; i++)
            {
                PassiveBuzzer_Set_Freq_Duty(400, 50);
                vTaskDelay(200);
                PassiveBuzzer_Set_Freq_Duty(600, 50);
                vTaskDelay(200);
            }
        }

        Fire_Alarm_Set(false);
        PassiveBuzzer_Control(0);
        vTaskDelay(500);
    }
}

void Key_Get_Task(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        // 为了原子性操作，不被其他任务干扰
        // taskENTER_CRITICAL();
        // ButtonHandler();
        // taskEXIT_CRITICAL();
        vTaskDelay(100);
    }
}
void My_Task_Init(void)
{
    xTaskCreate(EspLink_Task, "EspLink_Task", 256, NULL, 20, &xEspLinkTaskHandle);
    if (xEspLinkTaskHandle == NULL)
        printf("EspLink_Task create failed\r\n");
    xTaskCreate(My_Led_Task, "My_Led_Task", 256, NULL, 10, &xLedTaskHandle);
    if (xLedTaskHandle == NULL)
        printf("My_Led_Task create failed\r\n");
    xTaskCreate(HomePage_Task, "My_Home_Task", 256, NULL, 10, &xHomeTaskHandle);
    if (xHomeTaskHandle == NULL)
        printf("Home_Task create failed\r\n");
    xTaskCreate(Net_SendMsg_T, "SendMsg_Task", 256, NULL, 11, NULL);
    xTaskCreate(Net_RecvMsg_T, "RecvMsg_Task", 300, NULL, 11, NULL);
    xTaskCreate(Sensor_Task, "Sensor_Task", 256, NULL, 10, &xSensorTaskHandle);
    if (xSensorTaskHandle == NULL)
        printf("Sensor_Task create failed\r\n");
    xTaskCreate(Key_Get_Task, "Key_Get_Task", 128, NULL, 12, &xKeyGetHandle_t);
    if (xKeyGetHandle_t == NULL)
        printf("Key_Get_Task create failed\r\n");

    xAlarmEvent = xEventGroupCreate();
    if (xAlarmEvent == NULL)
        printf("xAlarmEvent create failed\r\n");

    xTaskCreate(Alarm_Process_Task, "Alarm_Process_Task", 128, NULL, 12, NULL);

    // 创建一个软件定时器
    // xTimerHandle_Key = xTimerCreate("KeyTimer", pdMS_TO_TICKS(1), pdTRUE, (void *)0, Key_TimerCallback);
    // if (xTimerHandle_Key == NULL)
    //     printf("KeyTimer create failed\r\n");
    // xTimerStart(xTimerHandle_Key, portMAX_DELAY);
}

void My_Drivers_Init(void)
{

    LED_Manager_Init();
    /*联网灯光//亮灭闪烁*/
    LedManager_SetLed_PulseS(0, 10, 10, 5);

    HAL_TIM_Base_Start_IT(&htim2);
    //  HAL_TIM_Base_Start(&htim1);
    HAL_UART_Receive_IT(&huart2, &chuan, 1);
    /* 需要在初始化时调用一次否则无法接收到内容 */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, Uart1RxBuffer, BUF_SIZE);
    PassiveBuzzer_Init();
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
    // Beep_GPIO_Init();
    OLED_Init();
    OLED_Clear();
    HAL_ADC_Start(&hadc1);
    HAL_ADC_Start(&hadc2);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1); // PWM_1
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2); // PWM_2

    // 从Flash加载窗口属性并恢复PWM设置
    Window_Flash_Init();
    // 从Flash加载阀门属性并恢复PWM设置
    Famen_Flash_Init();
    // __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 2000);

    MyRTC_SetTime(); // 设置时间

    My_Task_Init();
    Task_Tracker_Init(30 * 1000);
    //  测试git
}
