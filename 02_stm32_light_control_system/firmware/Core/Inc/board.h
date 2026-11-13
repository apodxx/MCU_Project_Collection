/**
  ******************************************************************************
  * @file    board.h
  * @brief   板级引脚定义与硬件开关（项目 02 · 智能光照控制）
  *
  * 接线（STM32F103C8T6 最小系统板）：
  *   光敏电阻模块 AO -> PA0    （ADC1_IN0，模拟量）
  *   电位器模块 C     -> PA1    （ADC1_IN1，旋钮调阈值）
  *   OLED SCL        -> PB10   （I2C2_SCL；接 PB6/I2C1 也能自动识别）
  *   OLED SDA        -> PB11   （I2C2_SDA；接 PB7/I2C1 也能自动识别）
  *   触摸模块 OUT     -> PA3    （TTP223，极性上电自动识别）
  *   继电器 IN        -> PB5    （低电平触发；高电平模块改 RELAY_ON_LEVEL）
  *   调试串口         -> PA9/PA10（USART1，115200）
  ******************************************************************************
  */
#ifndef BOARD_H
#define BOARD_H

#include "main.h"

/* ---------------------------- 光敏电阻 ---------------------------- */
/* 判断哪个方向是“暗”：
 *   1 = 遮住光敏电阻时数值变小（常见模块）
 *   0 = 遮住时数值变大（有些模块 AO 极性相反）
 * 上电后用手遮一下，看屏幕数值是变大还是变小，据此修改。 */
#define LIGHT_DARK_WHEN_LOW     1

/* ---------------------------- 电位器（阈值旋钮） ---------------------------- */
#define POT_ADC_CHANNEL         ADC_CHANNEL_1   /* PA1 */
#define POT_MIN_THRESHOLD       5u              /* 旋到最小对应 5% */
#define POT_MAX_THRESHOLD       95u             /* 旋到最大对应 95% */
/* 旋钮方向反了：把电位器的 A、B 两根线对调即可，不用改代码。 */

/* ---------------------------- 触摸按键 ---------------------------- */
#define TOUCH_PORT              GPIOA
#define TOUCH_PIN               GPIO_PIN_3
/* TTP223 有高有效/低有效两种，固件上电自动识别，这里只是初值。 */
#define TOUCH_ACTIVE_LEVEL      GPIO_PIN_SET

/* ---------------------------- 继电器 ---------------------------- */
#define RELAY_PORT              GPIOB
#define RELAY_PIN               GPIO_PIN_5
#define RELAY_ON_LEVEL          GPIO_PIN_RESET   /* 低电平触发；高电平模块改 GPIO_PIN_SET */
#define RELAY_OFF_LEVEL         ((RELAY_ON_LEVEL == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET)

/* ---------------------------- 板载 LED ---------------------------- */
#define LED_PORT                GPIOC
#define LED_PIN                 GPIO_PIN_13
#define LED_ON_LEVEL            GPIO_PIN_RESET
#define LED_OFF_LEVEL           ((LED_ON_LEVEL == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET)

/* ---------------------------- 业务参数 ---------------------------- */
#define APP_THRESHOLD_DEFAULT   30      /* 电位器还没读到时用的默认阈值 */
#define APP_HYSTERESIS          3       /* 回差 3%，防止继电器频繁动作 */

#endif /* BOARD_H */
