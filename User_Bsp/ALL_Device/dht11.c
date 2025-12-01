#include "dht11.h"
#include "FreeRTOS.h"
#include "task.h"

#define DHT11_PORT DHT11_PA11_GPIO_Port
#define DHT11_PIN DHT11_PA11_Pin
#define DHT11_TIMEOUT_RETRY 100
static uint8_t Rh_byte1, Rh_byte2, Temp_byte1, Temp_byte2, SUM;

uint8_t Presence = 0;

static uint8_t retry;
static uint8_t Response;

extern void vPortSetupTimerInterrupt(void);
void delay(uint32_t nus)
{
    uint32_t ticks;
    uint32_t told, tnow, reload, tcnt = 0;
    if ((0x0001 & (SysTick->CTRL)) == 0) // 定时器未工作
        vPortSetupTimerInterrupt();      // 初始化定时器

    reload = SysTick->LOAD;                    // 获取重装载寄存器值
    ticks = nus * (SystemCoreClock / 1000000); // 计数时间值

    vTaskSuspendAll();   // 阻止OS调度，防止打断us延时
    told = SysTick->VAL; // 获取当前数值寄存器值（开始时数值）
    while (1)
    {
        tnow = SysTick->VAL; // 获取当前数值寄存器值
        if (tnow != told)    // 当前值不等于开始值说明已在计数
        {
            if (tnow < told)         // 当前值小于开始数值，说明未计到0
                tcnt += told - tnow; // 计数值=开始值-当前值

            else                              // 当前值大于开始数值，说明已计到0并重新计数
                tcnt += reload - tnow + told; // 计数值=重装载值-当前值+开始值  （
                                              // 已从开始值计到0）

            told = tnow; // 更新开始值
            if (tcnt >= ticks)
                break; // 时间超过/等于要延迟的时间,则退出.
        }
    }
    xTaskResumeAll(); // 恢复OS调度
}

// SystemCoreClock为系统时钟(system_stmf4xx.c中)，通常选择该时钟作为
// systick定时器时钟，根据具体情况更改

void Set_Pin_Output(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

void Set_Pin_Input(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

/*温湿度模块dht11介绍
这是一个自带上拉电阻的模块，我测试了即使输入模式不加 上拉也没事
1、首先单片机要给dht11发送一个开始信号，首先拉低电平至少18ms，然后单片机输出26-28us的高电平
（这个20-40us的信号我觉得也是开始信号的一部分，表示单片机发送结束，去掉程序就不对了，但是有的解释很模糊）
2、dht11接受成功后，会发送一个持续80us的低电平信号，接着一个持续80us的高电平信号，表示准备完毕
3、dht11开始发送40bit的信号，每个bit都是以低电平开始，一会后拉高电平，再拉低，信号0：高电平持续20-40us
信号1；高电平持续70us，当发送完最后一次数据后，dht11会拉低电平50us，并拉起单总线电压，等待下一次开始信号
4、这40bit是五组数据8bit 湿度整数数据+8bit 湿度小数数据+8bit 温度整数数据+8bit 温度小数数据+8bit 校验和
校验和为前四个数据相加（应该是大小相加，我试了uint8_t, uint16_t, uint32_t 接收数据都正确，校验也正确），
则说明传输正确
*/

/*共用的单片机发送开始信号的代码*/
void DHT11_Start(void)
{
    Set_Pin_Output(DHT11_PORT, DHT11_PIN);                    // set the pin as output
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET); // pull the pin low
    delay(20000);                                             // wait for 18ms
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);   // pull the pin high
    delay(30);                                                // wait for 20us
}

/*检验是否连接成功-代码1-直接取值来判断*/
uint8_t DHT11_Check_Response(void)
{
    Response = 1;
    uint8_t timeout = 0;
    delay(40); // 又延时了40us，等待单总线被dht11拉低，因为单总线拉低80us，所以不会超范围
    Set_Pin_Input(DHT11_PORT, DHT11_PIN);
    if (!(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)))
    {
        delay(80); // 延时了80us，加上前面的40us，肯定大于dhtll拉低的80us，此时dhtll应该拉高了
                   // 并且dhtll拉高了80us，此时正好在拉高40us后，不会超范围
        if ((HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)))
            Response = 0;
        else
            Response = 255; // -1
        // 添加超时检测
        while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) && (++timeout < DHT11_TIMEOUT_RETRY))
        {
            delay(1); // 每次循环延时1us
        }
        if (timeout >= DHT11_TIMEOUT_RETRY)
        {
            Response = 255; // 超时错误码
        }
        return Response;
    }
    return Response;
}

/*读取驱动代码1*/
// 可以读取一位一位的数据，而不是一个字节
uint8_t DHT11_Read_Bit(void)
{
    uint16_t retry = 0;

    // 等待前导高电平结束（添加超时）
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET)
    {
        if (++retry > DHT11_TIMEOUT_RETRY)
            return 0xFF; // 返回错误码
        delay(1);        // 延时1us
    }

    retry = 0;
    // 等待低电平脉冲结束（添加超时）
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET)
    {
        if (++retry > DHT11_TIMEOUT_RETRY)
            return 0xFF;
        delay(1);
    }

    delay(40); // 关键采样点延时

    return (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) ? 1 : 0;
}

// 从DHT11读取一个字节
// 返回值：读取到的字节数据(8位)
uint8_t DHT11_Read_Byte(void)
{
    uint8_t dat = 0;
    for (uint8_t i = 0; i < 8; i++)
    {
        uint8_t bit = DHT11_Read_Bit();
        if (bit == 0xFF)
            return 0xFF; // 遇到超时立即返回
        dat = (dat << 1) | bit;
    }
    return dat;
}

/**********************************************************************
 * 函数名称： DHT11_Read
 * 功能描述： 读取DHT11的温度/湿度
 * 输入参数： 无
 * 输出参数： hum  - 用于保存湿度值
 *            temp - 用于保存温度值
 * 返 回 值： 1 - 成功, 0 - 失败
 ***********************************************************************/
uint8_t Read_dht11_Data(uint8_t *humi, uint8_t *temp)
{
    DHT11_Start();
    Presence = DHT11_Check_Response();
    Rh_byte1 = DHT11_Read_Byte();
    Rh_byte2 = DHT11_Read_Byte();
    Temp_byte1 = DHT11_Read_Byte();
    Temp_byte2 = DHT11_Read_Byte();
    SUM = DHT11_Read_Byte();
    /* 校验数据 */
    if (SUM == (Rh_byte1 + Rh_byte2 + Temp_byte1 + Temp_byte2))
    {
        *temp = Temp_byte1;
        *humi = Rh_byte1;
        return 1;
    }
    else
    {
        return 0;
    }
}

void DHT11Senser_Read(uint8_t *humi, uint8_t *temp)
{
    int8_t err;
    /* 读取DHT11的温湿度 */
    /* 暂停调度器 */
    vTaskSuspendAll();
    err = Read_dht11_Data(humi, temp);
    /* 恢复调度器 */
    xTaskResumeAll();

    if (err == 0)
    {
        *temp = 0;
        *humi = 0;
    }
}
