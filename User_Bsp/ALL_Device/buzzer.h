#ifndef _BUZZER_H
#define _BUZZER_H
#include "stdbool.h"

/**********************************************************************
 * 函数名称： PassiveBuzzer_Init
 * 功能描述： 无源蜂鸣器的初始化函数
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 无
 ***********************************************************************/
void PassiveBuzzer_Init(void);

/**********************************************************************
 * 函数名称： PassiveBuzzer_Control
 * 功能描述： 无源蜂鸣器控制函数
 * 输入参数： on - 1-响, 0-不响
 * 输出参数： 无
 * 返 回 值： 无
 ***********************************************************************/
void PassiveBuzzer_Control(int on);

/**********************************************************************
 * 函数名称： PassiveBuzzer_Set_Freq_Duty
 * 功能描述： 无源蜂鸣器控制函数: 设置频率和占空比
 * 输入参数： freq - 频率, duty - 占空比
 * 输出参数： 无
 * 返 回 值： 无
 ***********************************************************************/
void PassiveBuzzer_Set_Freq_Duty(int freq, int duty);

/**********************************************************************
 * 函数名称： PassiveBuzzer_Test
 * 功能描述： 无源蜂鸣器测试程序
 * 输入参数： 无
 * 输出参数： 无
 *            无
 * 返 回 值： 无
 ***********************************************************************/
void PassiveBuzzer_Test(void);

void Fire_Alarm_Set(bool enable);
void Fire_Alarm_Process(void);
void Beep_GPIO_Init(void);
void Beep_OnOff(bool on);
extern bool beep_status;

#endif /* _BUZZER_H */
