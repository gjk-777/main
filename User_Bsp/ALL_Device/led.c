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

void HandleSingleClick()
{
    // 蜂鸣器响
    Beep_OnOff(0);
    printf("单击\r\n");
    Led_Set(0);
}

void HandleDoubleClick()
{
    // 处理双击事件
    // 蜂鸣器响
    Beep_OnOff(0);
    printf("双击\r\n");
}

void HandleLongPress()
{
    // 处理长按事件
    // 蜂鸣器响
    Beep_OnOff(1);
    printf("长按\r\n");
    Led_Set(1);
}

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

void ButtonHandler(void)
{
    uint8_t curKeyState = HAL_GPIO_ReadPin(Key1_GPIO_Port, Key1_Pin);
    uint32_t currentTime = xTaskGetTickCount();

    switch (buttonState)
    {
    case BUTTON_IDLE:
        // 初始状态，等待按键按下
        isLongPress = false;
        clickCount = 0;
        if (curKeyState == GPIO_PIN_RESET) // 按键按下（低电平有效）
        {
            debounceTime = currentTime;
            buttonState = BUTTON_DEBOUNCE_PRESS;
        }
        break;

    case BUTTON_DEBOUNCE_PRESS:
        // 按下防抖
        if (currentTime - debounceTime >= DEBOUNCE_TIME)
        {
            if (curKeyState == GPIO_PIN_RESET) // 确认按键按下
            {
                buttonPressTime = currentTime;
                buttonState = BUTTON_PRESSED;
            }
            else
            {
                // 抖动，回到初始状态
                buttonState = BUTTON_IDLE;
            }
        }
        break;

    case BUTTON_PRESSED:
        // 按键已按下，检测长按或松开
        if (curKeyState == GPIO_PIN_SET) // 按键松开
        {
            debounceTime = currentTime;
            buttonState = BUTTON_DEBOUNCE_RELEASE;
        }
        else if (currentTime - buttonPressTime >= LONG_PRESS_TIME)
        {
            // 长按事件
            HandleLongPress();
            isLongPress = true;
            buttonState = BUTTON_IDLE; // 长按处理后直接回到初始状态
        }
        break;

    case BUTTON_DEBOUNCE_RELEASE:
        // 松开防抖
        if (currentTime - debounceTime >= DEBOUNCE_TIME)
        {
            if (curKeyState == GPIO_PIN_SET) // 确认按键松开
            {
                buttonReleaseTime = currentTime;

                if (isLongPress)
                {
                    // 如果已经处理了长按事件，直接回到初始状态
                    buttonState = BUTTON_IDLE;
                }
                else
                {
                    clickCount++;

                    if (clickCount == 1)
                    {
                        // 第一次点击，进入双击等待状态
                        buttonState = BUTTON_WAIT_FOR_DOUBLE;
                    }
                    else if (clickCount == 2)
                    {
                        // 第二次点击，触发双击事件
                        HandleDoubleClick();
                        buttonState = BUTTON_IDLE;
                    }
                }
            }
            else
            {
                // 抖动，回到按下状态
                buttonState = BUTTON_PRESSED;
            }
        }
        break;

    case BUTTON_WAIT_FOR_DOUBLE:
        // 等待双击的时间窗口
        if (curKeyState == GPIO_PIN_RESET) // 第二次点击按下
        {
            debounceTime = currentTime;
            buttonState = BUTTON_DEBOUNCE_PRESS; // 进入第二次点击的按下防抖
        }
        else if (currentTime - buttonReleaseTime >= DOUBLE_CLICK_TIME)
        {
            // 超过双击时间间隔，触发单击事件
            HandleSingleClick();
            buttonState = BUTTON_IDLE;
        }
        break;
    }
}
