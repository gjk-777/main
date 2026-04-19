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
extern bool fan_status;
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
// 修改版本为：260420.01
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

#define APP_WINDOW_TEMP_SAFE_ANGLE 60
#define APP_WINDOW_DANGER_SAFE_ANGLE 90
#define APP_FAMEN_CLOSE_ANGLE 0

typedef enum
{
    APP_ALARM_LED_OFF = 0,
    APP_ALARM_LED_TEMP,
    APP_ALARM_LED_CO,
    APP_ALARM_LED_MQ2,
    APP_ALARM_LED_FIRE,
} AppAlarmLedMode_t;

static void App_SetBuzzerSafe(bool enable)
{
    if (beep_status == enable)
    {
        return;
    }

    if (enable)
    {
        PassiveBuzzer_Set_Freq_Duty(1000, 50);
    }
    else
    {
        PassiveBuzzer_Control(0);
    }

    beep_status = enable;
    printf("[Control] Buzzer %s\r\n", enable ? "ON" : "OFF");
}

static void App_SetAlarmLedPattern(EventBits_t alarm_bits)
{
    static AppAlarmLedMode_t current_mode = APP_ALARM_LED_OFF;
    AppAlarmLedMode_t target_mode = APP_ALARM_LED_OFF;

    if (alarm_bits & EVENT_FIRE_BITS)
    {
        target_mode = APP_ALARM_LED_FIRE;
    }
    else if (alarm_bits & EVENT_MQ2_BITS)
    {
        target_mode = APP_ALARM_LED_MQ2;
    }
    else if (alarm_bits & EVENT_CO_MQ7_BITS)
    {
        target_mode = APP_ALARM_LED_CO;
    }
    else if (alarm_bits & EVENT_TEMP_BITS)
    {
        target_mode = APP_ALARM_LED_TEMP;
    }

    if (current_mode == target_mode)
    {
        return;
    }

    current_mode = target_mode;
    switch (target_mode)
    {
    case APP_ALARM_LED_FIRE:
        LedManager_SetLed_Blink(1, 100, 1, 1);
        printf("[Control] Alarm LED FIRE blink\r\n");
        break;

    case APP_ALARM_LED_MQ2:
        LedManager_SetLed_Blink(1, 100, 2, 2);
        printf("[Control] Alarm LED MQ2 blink\r\n");
        break;

    case APP_ALARM_LED_CO:
        LedManager_SetLed_Blink(1, 100, 4, 4);
        printf("[Control] Alarm LED CO blink\r\n");
        break;

    case APP_ALARM_LED_TEMP:
        LedManager_SetLed_Blink(1, 100, 7, 7);
        printf("[Control] Alarm LED TEMP blink\r\n");
        break;

    case APP_ALARM_LED_OFF:
    default:
        LedManager_SetLed_OnOff(1, false);
        printf("[Control] Alarm LED OFF\r\n");
        break;
    }
}

static void App_SetFanSafe(bool enable)
{
    if (fan_status != enable)
    {
        Fan_Set(enable);
        printf("[Control] Fan %s\r\n", enable ? "ON" : "OFF");
        /*
                printf("[控制] 风扇%s\r\n", enable ? "开启" : "关闭");
        */
    }
}

static void App_EnsureWindowSafe(uint8_t min_angle)
{
    if (window_angle_status < min_angle)
    {
        Servo_angle(min_angle);
        printf("[Control] Window -> %d\r\n", min_angle);
        /*
                printf("[控制] 窗户调整到 %d 度\r\n", min_angle);
        */
    }
}

static void App_CloseWindowSafe(void)
{
    if (window_angle_status != 0)
    {
        Servo_angle(0);
        printf("[Control] Window -> 0\r\n");
    }
}

static void App_CloseFamenSafe(void)
{
    if (famen_angle_status != APP_FAMEN_CLOSE_ANGLE)
    {
        Famen_angle(APP_FAMEN_CLOSE_ANGLE);
        printf("[Control] Famen -> 0\r\n");
        /*
                printf("[控制] 阀门关闭\r\n");
        */
    }
}

/**
 * @brief 安全联动控制 - 根据报警类型执行差异化安全策略
 *
 * @param alarm_bits 当前报警事件位
 *
 * 安全策略(参考现实消防常识):
 *
 * | 场景     | 风扇   | 窗户    | 阀门(燃气) | 原因                          |
 * |----------|--------|---------|------------|-------------------------------|
 * | 火灾     | 关闭   | 90°排烟 | 关闭       | 送风助燃!必须关风扇;关阀断气  |
 * | 甲烷泄漏 | 关闭   | 90°通风 | 关闭       | 风扇电机火花可能引爆甲烷      |
 * | 烟雾泄漏 | 开启   | 90°排烟 | 关闭       | 烟雾无明火,风扇排烟           |
 * | 温度异常 | 开启   | 60°散热 | 不动       | 散热为主,无泄漏风险           |
 * | 全部正常 | 关闭   | 不动    | 不动       | 恢复常态                      |
 *
 * 优先级: 火灾 > 甲烷 > 烟雾 > 温度
 * 多个报警同时存在时,取最高优先级的安全策略
 */
static void App_ApplySafetyControl(EventBits_t alarm_bits)
{
    bool has_alarm = ((alarm_bits & (EVENT_TEMP_BITS | EVENT_MQ2_BITS | EVENT_CO_MQ7_BITS | EVENT_FIRE_BITS)) != 0U);

    App_SetBuzzerSafe(has_alarm);
    App_SetAlarmLedPattern(alarm_bits);

    if (!has_alarm)
    {
        /* 全部正常: 阀门和窗户不动(保留用户设置) */
        /* 风扇：如果APP手动开启则保持，否则关闭 */
        if (!fan_manual_mode || !fan_status)
        {
            App_SetFanSafe(false);
        }
        return;
    }

    /* === 火灾: 关风扇(防止助燃!) + 关阀断气 + 关窗隔氧 === */
    if (alarm_bits & EVENT_FIRE_BITS)
    {
        App_SetFanSafe(false);   /* 火灾必须关风扇! 送风=助燃 */
        fan_manual_mode = false; /* 危险报警清除手动模式 */
        App_CloseFamenSafe();    /* 关闭燃气阀门, 切断燃料 */
        App_CloseWindowSafe();   /* 关窗隔氧, 抑制火势 */
        return;
    }

    /* === 甲烷泄漏: 开风扇排气 + 关阀断气 + 开窗通风 === */
    if (alarm_bits & EVENT_CO_MQ7_BITS)
    {
        App_SetFanSafe(true);                               /* 开风扇加速排出甲烷气体 */
        fan_manual_mode = false;                            /* 危险报警清除手动模式 */
        App_CloseFamenSafe();                               /* 关闭燃气阀门, 切断气源 */
        App_EnsureWindowSafe(APP_WINDOW_DANGER_SAFE_ANGLE); /* 开窗自然通风扩散 */
        return;
    }

    /* === 烟雾泄漏: 开风扇排烟 + 关阀断气 + 开窗排烟 === */
    if (alarm_bits & EVENT_MQ2_BITS)
    {
        App_SetFanSafe(true);                               /* 烟雾无明火, 风扇加速排烟 */
        App_CloseFamenSafe();                               /* 关闭燃气阀门, 切断气源 */
        App_EnsureWindowSafe(APP_WINDOW_DANGER_SAFE_ANGLE); /* 开窗排烟 */
        return;
    }

    /* === 温度异常: 开风扇散热 + 开窗通风 === */
    if (alarm_bits & EVENT_TEMP_BITS)
    {
        App_SetFanSafe(true);                             /* 温度高, 风扇散热 */
        App_EnsureWindowSafe(APP_WINDOW_TEMP_SAFE_ANGLE); /* 适度开窗通风 */
    }
}

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
    static uint8_t display_mode = DISPLAY_MODE_SENSOR; // 从传感器界面开始
    static uint8_t last_link_ok = 0;
    static uint8_t last_display_mode = 255;

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
                last_display_mode = 255; // 强制刷新
            }

            /* 更新传感器数据（每1.5秒更新一次） */
            if (count++ >= 30)
            {
                count = 0;
                DHT11Senser_Read(&humi, &temp);
            }

            /* 界面切换逻辑：每5秒切换一次 */
            if (switch_count++ >= 100) /* 100 * 50ms = 5秒 */
            {
                switch_count = 0;

                /* 保存当前模式 */
                uint8_t old_mode = display_mode;

                /* 切换到下一个界面 */
                display_mode = (display_mode == DISPLAY_MODE_SENSOR) ? DISPLAY_MODE_CONTROL : DISPLAY_MODE_SENSOR;

                /* 执行平滑过渡动画 */
                OLED_Smooth_Transition(old_mode, display_mode, &temp, &humi, &adc_mq2, &adc_CO_MQ7);

                last_display_mode = display_mode;
            }
            else
            {
                /* 正常刷新当前界面 */
                if (display_mode == DISPLAY_MODE_SENSOR)
                {
                    Data_Show(&temp, &humi, &adc_mq2, &adc_CO_MQ7);
                }
                else if (display_mode == DISPLAY_MODE_CONTROL)
                {
                    Control_Show();
                }
                OLED_Update();
            }

            vTaskDelay(50);
        }
        else
        {
            if (last_link_ok)
            {
                OLED_Clear();
                last_link_ok = 0;
                switch_count = 0; // 重置切换计数
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

            /* 3. 初始化本次扫描的报警位 */
            EventBits_t alarm_bits = 0;

            /* 4. 设置人体活动事件位 */
            if (body_status)
            {
                alarm_bits |= EVENT_BODY_BITS;
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
                    alarm_bits |= EVENT_TEMP_BITS;
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
                alarm_bits |= EVENT_MQ2_BITS;
            }

            /* 8. 甲烷浓度检测 */
            if (adc_CO_MQ7 > co_threshold)
            {
                const char *mode_str = cooking_status ? "做饭模式" : (body_status ? "有人" : "无人");
                printf("[报警] 甲烷浓度异常：%.2fppm (阈值: %.0fppm, %s)\r\n",
                       adc_CO_MQ7, co_threshold, mode_str);
                alarm_bits |= EVENT_CO_MQ7_BITS;
            }

            /* 9. 火灾检测（无论有人无人都必须报警） */
            Get_Fire_State();
            if (fire_status)
            {
                printf("[报警] 火灾检测到！\r\n");
                alarm_bits |= EVENT_FIRE_BITS;
            }
            /* 10. 增量更新事件组: 只清除不再活跃的事件位，只设置新的事件位
             * 避免先清后设导致的报警闪烁/恢复判断时序问题
             * - alarm_bits中为1的位: 设置(报警活跃)
             * - alarm_bits中为0的位: 清除(报警已解除)
             */
            EventBits_t current_event_bits = xEventGroupGetBits(xAlarmEvent) & EVENT_ALL_BITS;
            EventBits_t bits_to_clear = current_event_bits & ~alarm_bits; /* 当前活跃但新扫描不活跃的位 */
            EventBits_t bits_to_set = alarm_bits & ~current_event_bits;   /* 新扫描活跃但当前未设置的位 */

            if (bits_to_clear)
                xEventGroupClearBits(xAlarmEvent, bits_to_clear);
            if (bits_to_set)
                xEventGroupSetBits(xAlarmEvent, bits_to_set);

            /* 11. 打印当前状态 */
            printf("[状态] 做饭:%s | 人体:%s | 温度:%d℃ | 甲烷:%.1fppm(阈值:%.1f) | 烟雾:%.1f%%(阈值:%.1f)\r\n",
                   cooking_status ? "开" : "关",
                   body_status ? "有人" : "无人",
                   temp, adc_CO_MQ7, co_threshold, adc_mq2, mq2_threshold);

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
 * @brief 报警处理任务 - 事件组驱动架构
 *
 * @note 监听事件组 xAlarmEvent，根据不同报警类型执行对应的声光报警和安全控制
 *
 * 执行流程:
 * 1. xEventGroupWaitBits() 阻塞等待任意报警事件位（不自动清除）
 * 2. 读取当前事件位，执行 App_ApplySafetyControl() 安全联动控制
 *    - 蜂鸣器/风扇/报警灯/窗户/阀门等
 * 3. 根据事件优先级执行差异化声光报警:
 *    - 火警(EVENT_FIRE_BITS): 渐变频率报警，最紧急
 *    - 烟雾(EVENT_MQ2_BITS): 110断续音报警
 *    - 甲烷(EVENT_CO_MQ7_BITS): 110断续音报警
 *    - 温度(EVENT_TEMP_BITS): 双音交替报警
 * 4. 报警期间持续轮询事件位，若所有报警清除则恢复正常
 *
 * 事件位定义:
 *    bit0: EVENT_TEMP_BITS   温度异常
 *    bit1: EVENT_MQ2_BITS    烟雾浓度异常
 *    bit2: EVENT_CO_MQ7_BITS 甲烷浓度异常
 *    bit3: EVENT_FIRE_BITS   火灾
 *    bit4: EVENT_BODY_BITS   人体活动
 */
void Alarm_Process_Task(void *pvParameters)
{
    (void)pvParameters;
    bool is_alarming = false; /* 当前是否处于报警状态 */

    for (;;)
    {
        if (!is_alarming)
        {
            /* === 待机态：阻塞等待任意报警事件 ===
             * xEventGroupWaitBits 参数说明:
             *   xAlarmEvent     - 事件组句柄
             *   EVENT_ALL_BITS  - 等待的事件位掩码
             *   pdFALSE         - 退出时不清除事件位(由Sensor_Task管理)
             *   pdFALSE         - 等待任意一个事件位即可(逻辑或)
             *   portMAX_DELAY   - 无限等待
             */
            EventBits_t bits = xEventGroupWaitBits(
                xAlarmEvent,
                EVENT_ALL_BITS,
                pdFALSE,
                pdFALSE,
                portMAX_DELAY);

            /* 被唤醒，过滤有效报警位 */
            bits &= EVENT_ALL_BITS;
            if (bits == 0)
                continue;

            is_alarming = true;
            printf("[报警] 事件触发，进入报警态，事件位: 0x%02X\r\n", (unsigned int)bits);

            /* 执行安全联动控制: 蜂鸣器/风扇/报警灯/窗户/阀门 */
            App_ApplySafetyControl(bits);

            /* 报警开始：同步蜂鸣器状态到APP(OneNET上报) */
            beep_status = true;
            printf("[报警] 蜂鸣器状态已同步到APP: ON\r\n");
        }

        /* === 报警态：执行声光报警 + 持续监测 === */
        EventBits_t current_bits = xEventGroupGetBits(xAlarmEvent) & EVENT_ALL_BITS;

        if (current_bits == 0)
        {
            /* 所有报警已清除，恢复正常 */
            is_alarming = false;

            /* 关闭蜂鸣器，停止报警灯闪烁并熄灭 */
            PassiveBuzzer_Control(0);
            LedManager_SetLed_OnOff(1, false);

            /* 执行安全恢复控制(关闭蜂鸣器/风扇/报警灯) */
            App_ApplySafetyControl(0);

            /* 同步蜂鸣器状态到APP */
            beep_status = false;
            printf("[报警] 所有报警已清除，恢复正常\r\n");
            continue;
        }

        /* 报警仍在持续，根据优先级执行差异化声光报警 */
        if (current_bits & EVENT_FIRE_BITS)
        {
            /* 火警 - 最高优先级，渐变频率报警 */
            printf("[报警] 火警触发！执行火警频率报警\r\n");
            LedManager_SetLed_Blink(1, 100, 3, 3);
            Alarm_Fire_Sound(3);
        }
        else if (current_bits & EVENT_MQ2_BITS)
        {
            /* 烟雾浓度超标 - 110断续音报警 */
            printf("[报警] 烟雾浓度超标！执行110声音报警\r\n");
            LedManager_SetLed_Blink(1, 100, 5, 5);
            Alarm_110_Sound(2);
        }
        else if (current_bits & EVENT_CO_MQ7_BITS)
        {
            /* 甲烷浓度超标 - 110断续音报警 */
            printf("[报警] 甲烷浓度超标！执行110声音报警\r\n");
            LedManager_SetLed_Blink(1, 100, 5, 5);
            Alarm_110_Sound(2);
        }
        else if (current_bits & EVENT_TEMP_BITS)
        {
            /* 温度异常 - 双音交替报警 */
            printf("[报警] 温度异常！执行温度报警\r\n");
            LedManager_SetLed_Blink(1, 100, 8, 8);
            for (int i = 0; i < 3; i++)
            {
                PassiveBuzzer_Set_Freq_Duty(1000, 50);
                vTaskDelay(150);
                PassiveBuzzer_Set_Freq_Duty(600, 50);
                vTaskDelay(150);
            }
        }

        /* 更新安全联动控制(报警类型可能变化) */
        App_ApplySafetyControl(current_bits);

        /* 短暂延时，避免CPU占用过高，同时保持报警响应性 */
        vTaskDelay(pdMS_TO_TICKS(50));
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
    AlarmLedOFF();

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
