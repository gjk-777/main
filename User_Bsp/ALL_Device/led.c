#include "led.h"
#include "main.h"
#include "led_manager.h"
#include "tim.h"
#include "buzzer.h"
#include "use_boot_flash.h"

bool led_status = false;   /* 照明灯状态 PB14 */
bool fire_status = false;
bool body_status = false;
bool cooking_status = false;
bool fan_status = false;   /* 风扇状态 PB15 */
bool fan_manual_mode = false; /* 风扇手动模式标志 */
uint8_t window_angle_status = 0;
uint8_t famen_angle_status = 0;

/**
 * @brief 照明灯控制 PB14
 * @param status true-开灯, false-关灯
 *
 * 执行流程:
 * 1. 更新led_status全局状态变量
 * 2. 通过led_manager通道0控制PB14引脚电平
 *   - 低电平(RESET)=点亮, 高电平(SET)=熄灭
 */
void Led_Set(_Bool status)
{
    led_status = status;
    LedManager_SetLed_OnOff(0, status); /* 通道0: 照明灯 */
}

/**
 * @brief 风扇控制 PB15
 * @param status true-开启, false-关闭
 *
 * 执行流程:
 * 1. 更新fan_status全局状态变量
 * 2. 直接控制PB15引脚电平
 *   - 低电平(RESET)=开启, 高电平(SET)=关闭
 */
void Fan_Set(_Bool status)
{
    fan_status = status;
    if (status)
        FanMotor_ON();
    else
        FanMotor_OFF();
}

/**
 * @brief led_manager硬件控制回调函数
 * @param channel LED通道号
 * @param onOff true-点亮, false-熄灭
 *
 * 通道分配:
 *   通道0: 照明灯 PB14 (Fan_Pin)
 *   通道1: 报警灯 PB13 (LED_Beep_Pin)
 *
 * 引脚电平: 低电平有效(点亮)
 */
void Hardware_Led_Control(uint8_t channel, bool onOff)
{
    switch (channel)
    {
    case 0: /* 照明灯 PB14 */
        if (onOff)
            ZhaoMingLED_ON();
        else
            ZhaoMingLED_OFF();
        break;

    case 1: /* 报警灯 PB13 */
        if (onOff)
            AlarmLedON();
        else
            AlarmLedOFF();
        break;

    default:
        break;
    }
}

/**
 * @brief 初始化LED管理器
 *
 * 执行流程:
 * 1. 调用LedManager_Init注册2个LED通道和硬件控制回调
 * 2. 通道0: 照明灯(PB14), 通道1: 报警灯(PB13)
 * 3. 初始化后所有LED默认关闭
 */
void LED_Manager_Init(void)
{
    LedManager_Init(2, Hardware_Led_Control);
}

// 使用LED管理器控制LED示例
void LED_Manager_Usage(void)
{
    // 1. 打开LED0
    // LedManager_SetLed_OnOff(0, true);
    // HAL_Delay(2000);

    //  2. 关闭LED0
    // LedManager_SetLed_OnOff(0, false);
    // HAL_Delay(2000);

    // 3. 设置LED0闪烁：100ms为基础单位，亮3个单位(300ms)，灭5个单位(500ms)
    // LedManager_SetLed_Blink(0, 100, 3, 5);

    // 4. 如果需要使用脉冲功能，需要在led_manager.h中添加声明
    // LedManager_SetLed_Pulse(0, 500); // 500ms单次脉冲
    /*// 100ms为基础单位，亮5个单位(500ms)，灭12个单位(1200ms)*/
    // LedManager_SetLed_PulseS(0, 10, 5, 12);
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

/**
 * @brief 设置窗户角度（使用TIM3_CHANNEL_1）
 * @param angle 窗户角度 (0-90度)
 * @param save  是否保存到Flash（上电恢复时传false避免重复写入）
 *
 * 执行流程：
 * 1. 参数校验，角度限制在0-90度
 * 2. 计算PWM占空比值
 * 3. 设置TIM3通道1的比较值
 * 4. 根据save参数决定是否保存到Flash
 */
void Servo_angle_ex(uint8_t angle, bool save)
{
    if (angle > 90)
        angle = 90;
    uint32_t compare_value = 500 + (angle * 2000 / 180);
    window_angle_status = angle;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, compare_value);

    if (save)
    {
        Window_Flash_Save(angle);
    }
}

/**
 * @brief 设置窗户角度（兼容旧接口，默认保存Flash）
 */
void Servo_angle(uint8_t angle)
{
    Servo_angle_ex(angle, true);
}

/**
 * @brief 设置阀门角度（使用TIM3_CHANNEL_2）
 * @param angle 阀门角度 (0-90度)
 * @param save  是否保存到Flash（上电恢复时传false避免重复写入）
 *
 * 执行流程：
 * 1. 参数校验，角度限制在0-90度
 * 2. 计算PWM占空比值
 * 3. 设置TIM3通道2的比较值
 * 4. 根据save参数决定是否保存到Flash
 */
void Famen_angle_ex(uint8_t angle, bool save)
{
    if (angle > 90)
        angle = 90;
    uint32_t compare_value = 500 + (angle * 2000 / 180);
    famen_angle_status = angle;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, compare_value);

    if (save)
    {
        Famen_Flash_Save(angle);
    }
}

/**
 * @brief 设置阀门角度（兼容旧接口，默认保存Flash）
 */
void Famen_angle(uint8_t angle)
{
    Famen_angle_ex(angle, true);
}

// void HandleSingleClick()
// {
//     // 蜂鸣器响
//     Beep_OnOff(0);
//     printf("单击\r\n");
//     Led_Set(0);
// }

// void HandleDoubleClick()
// {
//     // 处理双击事件
//     // 蜂鸣器响
//     Beep_OnOff(0);
//     printf("双击\r\n");
// }

// void HandleLongPress()
// {
//     // 处理长按事件
//     // 蜂鸣器响
//     Beep_OnOff(1);
//     printf("长按\r\n");
//     Led_Set(1);
// }

#define DEBOUNCE_TIME 50      // 防抖时间（ms）
#define SINGLE_CLICK_TIME 500 // 单击最大时间间隔（ms）
#define DOUBLE_CLICK_TIME 500 // 双击最大时间间隔（ms）
#define LONG_PRESS_TIME 1500  // 长按时间（ms）

typedef enum
{
    BUTTON_IDLE,             // 初始状态
    BUTTON_DEBOUNCE_PRESS,   // 按下防抖
    BUTTON_PRESSED,          // 按键按下
    BUTTON_DEBOUNCE_RELEASE, // 松开防抖
    BUTTON_WAIT_FOR_DOUBLE   // 等待双击
} ButtonState;

volatile ButtonState buttonState = BUTTON_IDLE;
volatile uint32_t buttonPressTime = 0;
volatile uint32_t buttonReleaseTime = 0;
volatile uint32_t debounceTime = 0;
volatile bool isLongPress = false; // 标记是否发生了长按
volatile uint8_t clickCount = 0;   // 点击计数

// void ButtonHandler(void)
//{
//     uint8_t curKeyState = HAL_GPIO_ReadPin(Key1_GPIO_Port, Key1_Pin);
//     uint32_t currentTime = xTaskGetTickCount();

//    switch (buttonState)
//    {
//    case BUTTON_IDLE:
//        // 初始状态，等待按键按下
//        isLongPress = false;
//        clickCount = 0;
//        if (curKeyState == GPIO_PIN_RESET) // 按键按下（低电平有效）
//        {
//            debounceTime = currentTime;
//            buttonState = BUTTON_DEBOUNCE_PRESS;
//        }
//        break;

//    case BUTTON_DEBOUNCE_PRESS:
//        // 按下防抖
//        if (currentTime - debounceTime >= DEBOUNCE_TIME)
//        {
//            if (curKeyState == GPIO_PIN_RESET) // 确认按键按下
//            {
//                buttonPressTime = currentTime;
//                buttonState = BUTTON_PRESSED;
//            }
//            else
//            {
//                // 抖动，回到初始状态
//                buttonState = BUTTON_IDLE;
//            }
//        }
//        break;

//    case BUTTON_PRESSED:
//        // 按键已按下，检测长按或松开
//        if (curKeyState == GPIO_PIN_SET) // 按键松开
//        {
//            debounceTime = currentTime;
//            buttonState = BUTTON_DEBOUNCE_RELEASE;
//        }
//        else if (currentTime - buttonPressTime >= LONG_PRESS_TIME)
//        {
//            // 长按事件
//            HandleLongPress();
//            isLongPress = true;
//            buttonState = BUTTON_IDLE; // 长按处理后直接回到初始状态
//        }
//        break;

//    case BUTTON_DEBOUNCE_RELEASE:
//        // 松开防抖
//        if (currentTime - debounceTime >= DEBOUNCE_TIME)
//        {
//            if (curKeyState == GPIO_PIN_SET) // 确认按键松开
//            {
//                buttonReleaseTime = currentTime;

//                if (isLongPress)
//                {
//                    // 如果已经处理了长按事件，直接回到初始状态
//                    buttonState = BUTTON_IDLE;
//                }
//                else
//                {
//                    clickCount++;

//                    if (clickCount == 1)
//                    {
//                        // 第一次点击，进入双击等待状态
//                        buttonState = BUTTON_WAIT_FOR_DOUBLE;
//                    }
//                    else if (clickCount == 2)
//                    {
//                        // 第二次点击，触发双击事件
//                        HandleDoubleClick();
//                        buttonState = BUTTON_IDLE;
//                    }
//                }
//            }
//            else
//            {
//                // 抖动，回到按下状态
//                buttonState = BUTTON_PRESSED;
//            }
//        }
//        break;

//    case BUTTON_WAIT_FOR_DOUBLE:
//        // 等待双击的时间窗口
//        if (curKeyState == GPIO_PIN_RESET) // 第二次点击按下
//        {
//            debounceTime = currentTime;
//            buttonState = BUTTON_DEBOUNCE_PRESS; // 进入第二次点击的按下防抖
//        }
//        else if (currentTime - buttonReleaseTime >= DOUBLE_CLICK_TIME)
//        {
//            // 超过双击时间间隔，触发单击事件
//            HandleSingleClick();
//            buttonState = BUTTON_IDLE;
//        }
//        break;
//    }
//}
