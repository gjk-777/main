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
extern bool beep_status; // 蜂鸣器状态（需要在报警时同步更新）
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
#define EVENT_BODY_BITS (0x01 << 4) /* 人体活动事件位 */
#define EVENT_ALL_BITS (EVENT_TEMP_BITS | EVENT_MQ2_BITS | EVENT_CO_MQ7_BITS | EVENT_FIRE_BITS | EVENT_BODY_BITS)

/* Kitchen safety application policy (application-layer only). */
#define APP_DATA_UPLOAD_PERIOD_MS 2000
#define APP_SENSOR_SCAN_PERIOD_MS 500

/* 做饭模式阈值：温度>50℃, 甲烷>200ppm, 烟雾>20% */
#define APP_TEMP_COOKING_THRESHOLD 50
#define APP_CO_COOKING_THRESHOLD 200.0f
#define APP_MQ2_COOKING_THRESHOLD 20.0f

/* 有人模式阈值：温度>40℃, 甲烷>150ppm, 烟雾>20% */
#define APP_TEMP_BODY_PRESENT_THRESHOLD 40
#define APP_CO_BODY_PRESENT_THRESHOLD 150.0f
#define APP_MQ2_BODY_PRESENT_THRESHOLD 20.0f

/* 无人模式阈值：温度>30℃, 甲烷>100ppm, 烟雾>10% */
#define APP_TEMP_BODY_ABSENT_THRESHOLD 30
#define APP_CO_BODY_ABSENT_THRESHOLD 100.0f
#define APP_MQ2_BODY_ABSENT_THRESHOLD 10.0f

static void Name_Show()
{
    OLED_ShowChinese(0, 0, "连接中");
    OLED_ShowChinese(0, 16, "宇");
    OLED_ShowChinese(0, 32, "宇");
    OLED_ShowChinese(0, 48, "宇");
    OLED_ShowChinese(115, 48, "郭");
}
// static char time_str[20];
// static char date_str[20];

void My_Led_Task(void *pvParameters)
{
    (void)pvParameters;
    /* 上电后不再强制归零，由Window_Flash_Init()/Famen_Flash_Init()从Flash恢复角度 */
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
    static uint8_t last_link_ok = 0;
    DHT11Senser_Read(&humi, &temp);
    while (1)
    {
        vTaskGetInfo(xEspLinkTaskHandle, &xEspLink_State, pdFALSE, eInvalid);
        if (xEspLink_State.eCurrentState == eSuspended)
        {
            if (!last_link_ok)
            {
                OLED_Clear();
                last_link_ok = 1;
            }
            if (count++ >= 30)
            {
                count = 0;
                DHT11Senser_Read(&humi, &temp);
            }
            Data_Show(&temp, &humi, &adc_mq2, &adc_CO_MQ7);
            OLED_Update();

            vTaskDelay(50);
        }
        else
        {
            if (last_link_ok)
            {
                OLED_Clear();
                last_link_ok = 0;
            }
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
    for (;;)
    {
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
        k = 0;
        while (OneNet_DevLink()) // 接入OneNET
        {
            if (++k >= 5)
            {
                Uart_printf(USART_DEBUG, "OneNET link failed, retry connect flow\r\n");
                break;
            }
            vTaskDelay(100);
        }
        OneNET_Subscribe(); // 订阅主题
        Uart_printf(USART_DEBUG, "---------------------------Subscribe，Successful\r\n");

        if (k >= 5)
        {
            vTaskDelay(1000);
            continue;
        }
        OneNet_SendData();
        LedManager_SetLed_OnOff(0, false);
        OLED_Clear();
        vTaskSuspend(NULL); // 挂起本任务
    }
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
            dataPtr = Get_xiafa_data(10);
            if (dataPtr != NULL)
            {
                OneNet_RevPro(dataPtr);
            }
        }
        vTaskDelay(50);
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
            /* 1. 读取传感器数据 */
            adc_mq2 = MQ2_GetPPM();
            adc_CO_MQ7 = CO_MQ7_GetPPM();

            /* 2. 检测人体状态（优先检测） */
            Body_State();

            /* 3. 清除所有事件位 */
            xEventGroupClearBits(xAlarmEvent, EVENT_ALL_BITS);

            /* 4. 设置人体活动事件位 */
            if (body_status)
            {
                xEventGroupSetBits(xAlarmEvent, EVENT_BODY_BITS);
            }

            /* 5. 根据人体状态和做饭模式确定阈值 */
            uint8_t temp_threshold;
            float mq2_threshold;
            float co_threshold;

            if (cooking_status)
            {
                /* 做饭模式：使用做饭阈值 */
                temp_threshold = APP_TEMP_COOKING_THRESHOLD;
                mq2_threshold = APP_MQ2_COOKING_THRESHOLD;
                co_threshold = APP_CO_COOKING_THRESHOLD;
            }
            else if (body_status)
            {
                /* 有人：正常灵敏度阈值 */
                temp_threshold = APP_TEMP_BODY_PRESENT_THRESHOLD;
                mq2_threshold = APP_MQ2_BODY_PRESENT_THRESHOLD;
                co_threshold = APP_CO_BODY_PRESENT_THRESHOLD;
            }
            else
            {
                /* 无人：降低灵敏度阈值，减少误报 */
                temp_threshold = APP_TEMP_BODY_ABSENT_THRESHOLD;
                mq2_threshold = APP_MQ2_BODY_ABSENT_THRESHOLD;
                co_threshold = APP_CO_BODY_ABSENT_THRESHOLD;
            }

            /* 6. 温度异常检测（有人时才报警） */
            if (temp > temp_threshold)
            {
                if (body_status || cooking_status)
                {
                    printf("[报警] 温度异常：%d℃ (阈值: %d℃, %s)\r\n",
                           temp, temp_threshold, cooking_status ? "做饭模式" : "有人在场");
                    xEventGroupSetBits(xAlarmEvent, EVENT_TEMP_BITS);
                }
                else
                {
                    printf("[提示] 温度异常：%d℃ (阈值: %d℃, 无人，不报警)\r\n", temp, temp_threshold);
                }
            }

            /* 7. 烟雾浓度检测 */
            if (adc_mq2 > mq2_threshold)
            {
                const char *mode_str = cooking_status ? "做饭模式" : (body_status ? "有人" : "无人");
                printf("[报警] 烟雾浓度异常：%.2f%% (阈值: %.0f%%, %s)\r\n",
                       adc_mq2, mq2_threshold, mode_str);
                xEventGroupSetBits(xAlarmEvent, EVENT_MQ2_BITS);
            }

            /* 8. 甲烷浓度检测 */
            if (adc_CO_MQ7 > co_threshold)
            {
                const char *mode_str = cooking_status ? "做饭模式" : (body_status ? "有人" : "无人");
                printf("[报警] 甲烷浓度异常：%.2fppm (阈值: %.0fppm, %s)\r\n",
                       adc_CO_MQ7, co_threshold, mode_str);
                xEventGroupSetBits(xAlarmEvent, EVENT_CO_MQ7_BITS);
            }

            /* 9. 火灾检测（无论有人无人都必须报警） */
            Get_Fire_State();
            if (fire_status)
            {
                printf("[报警] 火灾检测到！\r\n");
                xEventGroupSetBits(xAlarmEvent, EVENT_FIRE_BITS);
            }

            /* 10. 打印当前状态 */
            printf("[状态] 做饭:%s | 人体:%s | 温度:%d℃ | 甲烷:%.1fppm | 烟雾:%.1f%%\r\n",
                   cooking_status ? "开" : "关",
                   body_status ? "有人" : "无人",
                   temp, adc_CO_MQ7, adc_mq2);

            vTaskDelay(pdMS_TO_TICKS(APP_SENSOR_SCAN_PERIOD_MS));
        }
    }
}

/**
 * @brief 110报警声音模式 - 浓度超标报警
 * @param cycles 报警循环次数
 * @note 模拟110报警：急促的"滴-滴-滴"断续音，频率约800-1200Hz
 */
/**
 * @brief 110报警声音模式 - 浓度超标报警
 * @param cycles 报警循环次数
 * @note 报警灯由led_manager通道1异步闪烁管理，此处仅控制蜂鸣器
 */
static void Alarm_110_Sound(uint8_t cycles)
{
    for (uint8_t i = 0; i < cycles; i++)
    {
        /* 滴 - 高音 */
        PassiveBuzzer_Set_Freq_Duty(1000, 50);
        vTaskDelay(100);
        /* 静音间隙 */
        PassiveBuzzer_Control(0);
        vTaskDelay(50);
        /* 滴 - 高音 */
        PassiveBuzzer_Set_Freq_Duty(1000, 50);
        vTaskDelay(100);
        /* 静音间隙 */
        PassiveBuzzer_Control(0);
        vTaskDelay(50);
        /* 滴 - 高音 */
        PassiveBuzzer_Set_Freq_Duty(1000, 50);
        vTaskDelay(100);
        /* 长间隙 */
        PassiveBuzzer_Control(0);
        vTaskDelay(300);
    }
}

/**
 * @brief 火警报警声音模式
 * @param cycles 报警循环次数
 * @note 火警频率：连续的渐变音，表示紧急情况
 */
/**
 * @brief 火警报警声音模式
 * @param cycles 报警循环次数
 * @note 报警灯由led_manager通道1异步闪烁管理，此处仅控制蜂鸣器
 */
static void Alarm_Fire_Sound(uint8_t cycles)
{
    for (uint8_t i = 0; i < cycles; i++)
    {
        /* 频率上升阶段 */
        for (uint16_t freq = 600; freq <= 1200; freq += 50)
        {
            PassiveBuzzer_Set_Freq_Duty(freq, 50);
            vTaskDelay(20);
        }
        /* 频率下降阶段 */
        for (uint16_t freq = 1200; freq >= 600; freq -= 50)
        {
            PassiveBuzzer_Set_Freq_Duty(freq, 50);
            vTaskDelay(20);
        }
    }
}

/**
 * @brief 报警处理任务
 * @note 监听事件组，根据不同报警类型执行对应的声光报警
 *       - 火警：渐变频率报警（紧急）
 *       - 浓度超标(MQ2/CO)：110报警声音（急促断续音）
 *       - 温度异常：双音交替报警
 */
void Alarm_Process_Task(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        /* 阻塞等待任意报警事件，pdTRUE表示退出时自动清除事件位 */
        uint32_t bits = xEventGroupWaitBits(xAlarmEvent, EVENT_ALL_BITS, pdTRUE, pdFALSE, portMAX_DELAY);

        /* 报警开始：设置蜂鸣器状态为开启，同步到APP */
        beep_status = true;
        printf("[报警] 蜂鸣器状态已同步到APP: ON\r\n");

        /* 火警报警 - 最高优先级，使用渐变频率，报警灯急闪 */
        if (bits & EVENT_FIRE_BITS)
        {
            printf("[报警] 火警触发！执行火警频率报警\r\n");
            LedManager_SetLed_Blink(1, 100, 3, 3); /* 通道1报警灯急闪: 300ms亮+300ms灭 */
            Alarm_Fire_Sound(5);
        }
        /* 浓度超标报警 - MQ2烟雾，报警灯快闪 */
        else if (bits & EVENT_MQ2_BITS)
        {
            printf("[报警] 烟雾浓度超标！执行110声音报警\r\n");
            LedManager_SetLed_Blink(1, 100, 5, 5); /* 通道1报警灯快闪: 500ms亮+500ms灭 */
            Alarm_110_Sound(3);
        }
        /* 浓度超标报警 - CO甲烷，报警灯快闪 */
        else if (bits & EVENT_CO_MQ7_BITS)
        {
            printf("[报警] 甲烷浓度超标！执行110声音报警\r\n");
            LedManager_SetLed_Blink(1, 100, 5, 5); /* 通道1报警灯快闪: 500ms亮+500ms灭 */
            Alarm_110_Sound(3);
        }
        /* 温度异常报警，报警灯慢闪 */
        else if (bits & EVENT_TEMP_BITS)
        {
            printf("[报警] 温度异常！执行温度报警\r\n");
            LedManager_SetLed_Blink(1, 100, 8, 8); /* 通道1报警灯慢闪: 800ms亮+800ms灭 */
            for (int i = 0; i < 4; i++)
            {
                PassiveBuzzer_Set_Freq_Duty(1000, 50);
                vTaskDelay(150);
                PassiveBuzzer_Set_Freq_Duty(600, 50);
                vTaskDelay(150);
            }
        }
        /* 报警结束：关闭蜂鸣器，停止报警灯闪烁并熄灭 */
        PassiveBuzzer_Control(0);
        LedManager_SetLed_OnOff(1, false); /* 停止通道1闪烁并关闭报警灯 */

        /* 报警结束：设置蜂鸣器状态为关闭，同步到APP */
        beep_status = false;
        printf("[报警] 蜂鸣器状态已同步到APP: OFF\r\n");

        vTaskDelay(500);
    }
}

void Key_Get_Task(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(2000));
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

    xTaskCreate(Alarm_Process_Task, "Alarm_Process_Task", 256, NULL, 12, NULL);

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
    /* 上电关闭风扇PB15（gpio.c初始化为RESET低电平=开启，需显式关闭） */
    FanMotor_OFF();

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

    // 从Flash加载窗口属性并恢复PWM设置
    Window_Flash_Init();
    // 从Flash加载阀门属性并恢复PWM设置
    Famen_Flash_Init();
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1); // PWM_1
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2); // PWM_2
    // __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 2000);

    MyRTC_SetTime(); // 设置时间

    My_Task_Init();
    Task_Tracker_Init(30 * 1000);
    //  测试git
}
