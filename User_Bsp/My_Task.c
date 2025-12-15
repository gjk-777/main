#include "My_Task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "tim.h"
#include "cmsis_os.h"

#include "OLED.h"
#include "led.h"
#include "oled_display.h"
#include "buzzer.h"
#include "dht11.h"
#include "usart.h"
#include "string.h"
#include "stdio.h"
#include "esp8266.h"
#include "onenet.h"
#include "adc.h"
#include "w25q64.h"
#include "m24c02.h"
#include "queue.h"  //队列
#include "timers.h" //软件定时器
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
TaskHandle_t xSendMsgHandle_t;
TaskHandle_t xRecvMsgHandle_t;
TaskHandle_t xSensorTaskHandle;

void check_memory()
{
    printf("剩余堆内存: %d 字节\n", xPortGetFreeHeapSize());
}

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
    uint32_t i = 0;
    uint32_t TotalRunTime = 0;
    UBaseType_t task_num = 0;
    TaskStatus_t *status_array = NULL;
    TaskHandle_t task_handle = NULL;
    TaskStatus_t *task_info = NULL;
    eTaskState task_state = eInvalid;
    char *task_state_str = NULL;
    char *task_info_buf = NULL;
    /* 函数vTaskList()的使用*/
    printf("/*************第四步：函数vTaskList()的使用************/\r\n");
    task_info_buf = pvPortMalloc(256);
    if (task_info_buf == NULL)
    {
        /* code */
        printf("内存分配失败\r\n");
    }

    vTaskList(task_info_buf); /* 获取所有任务的信息 */
    printf("任务名\t\t状态\t优先级\t剩余栈\t任务序号\r\n");
    printf("%s\r\n", task_info_buf);
    vPortFree(task_info_buf);
    printf("/********************实验结束**********************/\r\n");

    while (1)
    {

        check_memory();
        Bsp_LedToggle();
        // printf("led亮\r\n");
        //  PassiveBuzzer_Test();
        Beep_OnOff(0);
        vTaskDelay(2000);
    }
}

void Home_Task(void *pvParameters)
{

    while (1)
    {
        OLED_Clear();
        TimeDisplay();
        /* 每5秒更新一次温湿度数据 */
        if (count++ == 2)
        {
            count = 0;
            DHT11Senser_Read(&humi, &temp);
        }
        Data_Show(&temp, &humi);
        // Name_Show();

        OLED_Update();
        vTaskDelay(500);
    }
}

void EspLink_Task(void *pvParameters)
{
    // M24C02_Test();
    // PassiveBuzzer_Test();
    /** */
    printf("tasktask\r\n");
    uint8_t k = 0;
    ESP8266_Init(); // 初始化ESP8266
    HAL_Delay(100);
    Uart_printf(USART_DEBUG, "Connect MQTTs Server...\r\n");
    Connect(ESP8266_ONENET_INFO, "CONNECT");
    HAL_Delay(50);
    Uart_printf(USART_DEBUG, "Connect MQTT Server Success--OKOKOKOK\r\n");
    while (OneNet_DevLink()) // 接入OneNET
    {
        if (k++ == 4)
            HAL_NVIC_SystemReset();
        HAL_Delay(500);
    }
    OneNET_Subscribe(); // 订阅主题
    Uart_printf(USART_DEBUG, "---------------------------Subscribe，Successful\r\n");
    vTaskSuspend(NULL); // 挂起本任务
}
void Net_SendMsg_T(void *pvParameters)
{
    for (;;)
    {
        OneNet_SendData(); // 发送数据
        ESP8266_Clear1_2();
        vTaskDelay(100);
    }
}
void Net_RecvMsg_T(void *pvParameters)
{
    for (;;)
    {
        dataPtr = Get_xiafa_data(2);
        if (dataPtr != NULL)
        {
            // fangchong = 1;
            OneNet_RevPro(dataPtr);
        }
        vTaskDelay(50);
    }
}
void Sensor_Task(void *pvParameters)
{
    for (;;)
    {
        adc_mq2 = MQ2_GetPPM(); // ADC采集程序
        adc_CO_MQ7 = CO_MQ7_GetPPM();
        if (adc_mq2 > 100.0f)
        {
            printf("mq2浓度异常：%.2fppm\r\n", adc_mq2);
            Beep_OnOff(1);
        }
        if (adc_CO_MQ7 > 100.0f)
        {
            printf("CO浓度异常：%.2fppm\r\n", adc_CO_MQ7);
            Beep_OnOff(1);
        }
        printf("CO浓度 ：%.2fppm\r\n", adc_CO_MQ7);
        printf("mq浓度 ：%.2fppm\r\n", adc_mq2);
        Get_Fire_State();
        Body_State();
        vTaskDelay(1000);
    }
}

void My_Task_Init(void)
{
    xTaskCreate(EspLink_Task, "EspLink_Task", 750, NULL, 15, &xEspLinkTaskHandle);
    if (xEspLinkTaskHandle == NULL)
        printf("EspLink_Task create failed\r\n");
    xTaskCreate(My_Led_Task, "My_Led_Task", 128, NULL, 10, &xLedTaskHandle);
    if (xLedTaskHandle == NULL)
        printf("My_Led_Task create failed\r\n");
    xTaskCreate(Home_Task, "My_Home_Task", 256, NULL, 10, &xHomeTaskHandle);
    if (xHomeTaskHandle == NULL)
        printf("Home_Task create failed\r\n");
    xTaskCreate(Net_SendMsg_T, "SendMsg_Task", 512, NULL, 11, &xSendMsgHandle_t);
    if (xSendMsgHandle_t == NULL)
        printf("SendMsg_Task create failed\r\n");
    xTaskCreate(Net_RecvMsg_T, "RecvMsg_Task", 512, NULL, 11, &xRecvMsgHandle_t);
    if (xRecvMsgHandle_t == NULL)
        printf("RecvMsg_Task create failed\r\n");
    xTaskCreate(Sensor_Task, "Sensor_Task", 256, NULL, 10, &xSensorTaskHandle);
    if (xSensorTaskHandle == NULL)
        printf("Sensor_Task create failed\r\n");
}

void My_Drivers_Init(void)
{
    HAL_TIM_Base_Start_IT(&htim2);
    //  HAL_TIM_Base_Start(&htim1);
    HAL_UART_Receive_IT(&huart2, &chuan, 1);
    /* 需要在初始化时调用一次否则无法接收到内容 */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, Uart1RxBuffer, BUF_SIZE);
    // PassiveBuzzer_Init();
    Beep_GPIO_Init();
    OLED_Init();
    OLED_Clear();
    HAL_ADC_Start(&hadc1);
    HAL_ADC_Start(&hadc2);
    My_Task_Init();
}
