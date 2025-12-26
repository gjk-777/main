#include "led_manager.h"

#include "FreeRTOS.h"
#include "timers.h"
#include "string.h"

/**
 * @note LED亮灭、闪烁、脉冲管理
 * 闪烁和脉冲通过RTOS的软件定时器实现。
 * 闪烁和脉冲的参数通过定时器的ID传递。具体参数编码规则如下
 * 定时器id为32bit，类型void* ，故可存储一个32bit数据，也可存储一个指针。 此处应用存储led控制结构体指针
 */

typedef enum edef_led_State
{
    LED_STATE_OFF = 0, // LED状态：熄灭
    LED_STATE_ON = 1,  // LED状态：点亮
    LED_STATE_BLINK,   // LED状态：闪烁
    LED_STATE_PULSE,   // LED状态：单次脉冲
    LED_STATE_PULSES,  // LED状态：多次脉冲
} e_Led_State;

#pragma pack(1)
typedef struct tdef_LedCtrlInfo // led信息
{
    uint8_t channel;          // LED通道
    uint16_t blinkPulseWidth; // 闪烁、脉冲宽度
    uint8_t blinkHigh;        // 闪烁高电平时间
    uint8_t blinkLow;         // 闪烁低电平时间
    uint16_t blinkCount;      // 闪烁计数
    TimerHandle_t led_timer;  // 定时器句柄
    e_Led_State estate;       // LED状态
    bool currentOnOff;        // 当前LED开关状态

} t_LedCtrlInfo;
#pragma pack()

static uint8_t prvLed_Count;                               // led数量
static t_LedCtrlInfo *pxLedInfo;                           // led信息控制结构体指针
static void (*prvHalLedFunc)(uint8_t channel, bool onOff); // led硬件HAL层控制入口函数指针

static void LedManager_TimerCallback(TimerHandle_t xTimer) // 定时器回调函数
{
    t_LedCtrlInfo *pxLed = pvTimerGetTimerID(xTimer);
    /* 闪烁状态进入定时器回调, 进行计数判断，决定led灯亮灭 */
    if (LED_STATE_BLINK == pxLed->estate)
    {
        pxLed->blinkCount++;
        if (pxLed->blinkCount == pxLed->blinkLow) /* 低计数结束 */
        {
            prvHalLedFunc(pxLed->channel, true);
        }
        else if (pxLed->blinkCount == pxLed->blinkHigh + pxLed->blinkLow) /* 高计数结束 */
        {
            prvHalLedFunc(pxLed->channel, false);
            pxLed->blinkCount = 0;
        }
    }
    else if (LED_STATE_PULSE == pxLed->estate) /* 脉冲状态进入定时器中断，led熄灭，并将状态变更为LED_STATE_OFF */
    {
        prvHalLedFunc(pxLed->channel, false);
        pxLed->estate = LED_STATE_OFF;
    }
    else if (LED_STATE_PULSES == pxLed->estate)
    {
        pxLed->blinkCount++;
        if (pxLed->blinkCount <= pxLed->blinkHigh)
        {
            if (pxLed->blinkCount % 2)
                prvHalLedFunc(pxLed->channel, true);
            else
                prvHalLedFunc(pxLed->channel, false);
        }
        else if (pxLed->blinkCount == pxLed->blinkHigh + pxLed->blinkLow)
        {
            pxLed->blinkCount = 0;
            prvHalLedFunc(pxLed->channel, false);
        }
    }
}

void LedManager_Init(uint8_t ledCount, void (*halLedFunc)(uint8_t channel, bool onOff))
{
    prvLed_Count = ledCount;
    prvHalLedFunc = halLedFunc;
    pxLedInfo = pvPortMalloc(sizeof(t_LedCtrlInfo) * prvLed_Count); // 分配内存空间并初始化
    memset(pxLedInfo, 0, sizeof(t_LedCtrlInfo) * prvLed_Count);
    // 创建软件定时器，使能LED，初始化led为关闭状态
    for (uint8_t i = 0; i < prvLed_Count; ++i)
    {
        t_LedCtrlInfo *pxLed = pxLedInfo + i;
        pxLed->channel = i;
        pxLed->currentOnOff = true;
        pxLed->led_timer = xTimerCreate("LED_Timer", pdMS_TO_TICKS(1000), pdFALSE, pxLed, LedManager_TimerCallback);
        prvHalLedFunc(pxLed->channel, false); /* 通道led初始状态：关闭 */
    }
}

void LedManager_SetLed_OnOff(uint8_t channel, bool onOff)
{
    if (channel >= prvLed_Count)
        return;

    t_LedCtrlInfo *pxLed = pxLedInfo + channel; // 获取led控制结构体属性指针
    if (pxLed->estate == onOff)                 // 如果状态与要设置的状态相同，则直接返回
        return;
    pxLed->estate = onOff ? LED_STATE_ON : LED_STATE_OFF; // 状态设置
    if (xTimerIsTimerActive(pxLed->led_timer))
    {
        xTimerStop(pxLed->led_timer, portMAX_DELAY);
    }
    /* 使能状态下，设置硬件Hal */
    prvHalLedFunc(channel, onOff);
}

void LedManager_SetLed_Blink(uint8_t channel, uint16_t blinkPulseWidth, uint8_t highCount, uint8_t lowCount)
{
    if (channel >= prvLed_Count || blinkPulseWidth == 0 || highCount == 0 || lowCount == 0)
        return;

    t_LedCtrlInfo *pxLed = pxLedInfo + channel; // 获取led控制结构体属性指针
    pxLed->estate = LED_STATE_BLINK;            // 状态设置
    pxLed->blinkPulseWidth = blinkPulseWidth;
    pxLed->blinkHigh = highCount;
    pxLed->blinkLow = lowCount;
    pxLed->blinkCount = 0;

    if (pxLed->currentOnOff)
    {
        /*设置定时器为周期模式，并设置周期为blinkPulseWidth，修改周期后，定时器将自动启动*/
        vTimerSetReloadMode(pxLed->led_timer, pdTRUE);
        xTimerChangePeriod(pxLed->led_timer, pdMS_TO_TICKS(pxLed->blinkPulseWidth), portMAX_DELAY);
    }
}

void LedManager_SetLed_Pulse(uint8_t channel, uint16_t pulseWidth)
{
    if (channel >= prvLed_Count || pulseWidth == 0)
        return;

    t_LedCtrlInfo *pxLed = pxLedInfo + channel; /* 获取led控制结构体属性指针*/
    /*设置blink参数*/
    pxLed->estate = LED_STATE_PULSE; // 状态设置为单次脉冲状态
    pxLed->blinkPulseWidth = pulseWidth;
    pxLed->blinkCount = 0;
    /*led使能状态下，开启LED*/
    if (pxLed->currentOnOff)
    {
        prvHalLedFunc(pxLed->channel, true);
        /*设置定时器为周期模式，并设置周期为pulseWidth，修改周期后，定时器将自动启动*/
        vTimerSetReloadMode(pxLed->led_timer, pdTRUE);
        xTimerChangePeriod(pxLed->led_timer, pdMS_TO_TICKS(pxLed->blinkPulseWidth), portMAX_DELAY);
    }
}

void LedManager_SetLed_PulseS(uint8_t channel, uint16_t pulseWidth, uint8_t highCount, uint8_t lowCount)
{
    if (channel >= prvLed_Count || pulseWidth == 0 || highCount == 0 || lowCount == 0)
        return;

    t_LedCtrlInfo *pxLed = pxLedInfo + channel; /* 获取led控制结构体属性指针*/
    /*设置blink参数*/
    pxLed->estate = LED_STATE_PULSES; // 状态设置为单次脉冲状态
    pxLed->blinkCount = 0;
    pxLed->blinkPulseWidth = pulseWidth;
    pxLed->blinkHigh = highCount * 2;
    pxLed->blinkLow = lowCount;
    if (pxLed->currentOnOff)
    {
        /* 设定定时器周期模式，并设置周期为pulseWidth **修改周期后，定时器将自动启动 */
        vTimerSetReloadMode(pxLed->led_timer, pdTRUE);
        xTimerChangePeriod(pxLed->led_timer, pdMS_TO_TICKS(pxLed->blinkPulseWidth), portMAX_DELAY);
    }
}
