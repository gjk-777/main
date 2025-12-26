/**
 * @file LedManager.h
 * @author
 * @brief LED管理
 * @version 1.0
 * @date 2025
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifndef __LED_MANAGER_H
#define __LED_MANAGER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"
#include "stdbool.h"

    /**
     * @brief LED管理 - 初始化 -- 设置通道数并分配空间
     *
     * @param ledCount led通道数量
     * @param halLedFunc led硬件控制函数，函数原型 void (*)(uint8_t channel, bool on)
     * @param halSuspendHandle led暂停模式下， 需要特殊处理的回调内容，函数原型 void (*) (bool suspend)
     */
    void LedManager_Init(uint8_t ledCount, void (*halLedFunc)(uint8_t channel, bool on));

    /**
     * @brief Led管理 - 开关控制
     *
     * @param channel 通道
     * @param onOff 开/关 bool类型
     */
    void LedManager_SetLed_OnOff(uint8_t channel, bool onOff);

    /**
     * @brief Led管理 - 闪烁
     *
     * @param channel 通道
     * @param pulseWidth 基础脉冲宽度（单位ms，亮灭的时间长度以此设定参数为倍数。例如pulseWidth=10, highCount=5, lowCount=12,则闪烁为 50ms亮+120ms灭 周期循环
     * @param highCount 脉冲亮周期数
     * @param lowCount 脉冲灭周期数
     */
    void LedManager_SetLed_Blink(uint8_t channel, uint16_t pulseWidth, uint8_t highCount, uint8_t lowCount);

    /**
     * @brief Led管理 - 多次脉冲
     *
     * @param channel 通道
     * @param pulseWidth 基础脉冲宽度（单位ms，亮灭的时间长度以此设定参数为倍数。例如pulseWidth=10, highCount=5, lowCount=12,则闪烁为 50ms亮+120ms灭 周期循环
     * @param highCount 脉冲亮周期数
     * @param lowCount 脉冲灭周期数
     */
    void LedManager_SetLed_PulseS(uint8_t channel, uint16_t pulseWidth, uint8_t highCount, uint8_t lowCount);

    void LedManager_SetLed_Pulse(uint8_t channel, uint16_t pulseWidth);

#ifdef __cplusplus
}
#endif

#endif /* __LED_MANAGER_H */
