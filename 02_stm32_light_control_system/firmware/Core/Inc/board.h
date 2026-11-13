/**
  ******************************************************************************
  * @file    board.h
  * @brief   板级引脚定义与硬件开关（项目 02 · 智能光照控制）
  *
  * 接线（STM32F103C8T6 最小系统板）：
  *   光敏电阻模块 AO -> PA0    （ADC1_IN0，模拟量）
  *   旋转编码器 A     -> PA1
  *   旋转编码器 B     -> PA2
  *   旋转编码器 C     -> GND    （公共端）
  *   OLED SCL        -> PB10   （I2C2_SCL；接 PB6/I2C1 也能自动识别）
  *   OLED SDA        -> PB11   （I2C2_SDA；接 PB7/I2C1 也能自动识别）
  *   触摸模块 OUT     -> PA3    （TTP223，极性上电自动识别）
  *   执行器           -> PB5    （默认接 LED；换继电器只改一个宏）
  *   调试串口         -> PA9/PA10（USART1，115200）
  ******************************************************************************
  */
#ifndef BOARD_H
#define BOARD_H

#include "main.h"

/* ---------------------------- 光敏电阻 ---------------------------- */
/* 判断哪个方向是“暗”：
 *   1 = 遮住光敏电阻时数值变小
 *   0 = 遮住时数值变大（你手上这块模块是这种：越暗输出越高）
 * 上电后用手遮一下、再用手电照一下，看屏幕数值变化方向，据此修改。 */
#define LIGHT_DARK_WHEN_LOW     0

/* ---------------------------- 旋转编码器（360 度，A/C/B 三脚） ---------------------------- */
#define ENC_A_PORT              GPIOA
#define ENC_A_PIN               GPIO_PIN_1
#define ENC_B_PORT              GPIOA
#define ENC_B_PIN               GPIO_PIN_2

/* 转一格跳变数：不同编码器每格 2~4 个跳变，转一格跳两下就把 4 改成 8，转不动就改成 2 */
#define ENC_COUNTS_PER_STEP     4
/* 方向反了：A/B 两根线对调，或把这里改成 1 */
#define ENC_REVERSE             0

/* ---------------------------- 触摸按键 ---------------------------- */
#define TOUCH_PORT              GPIOA
#define TOUCH_PIN               GPIO_PIN_3
/* TTP223 有高有效/低有效两种，固件上电自动识别，这里只是初值。 */
#define TOUCH_ACTIVE_LEVEL      GPIO_PIN_SET

/* ---------------------------- 执行器：LED / 继电器 / 蜂鸣器 ---------------------------- */
/* 手头暂时没有继电器模块，默认用 LED 指示：
 *   PB5 ── 330Ω ── LED 正极，LED 负极 ── GND
 * 以后买到继电器模块，把 ACTUATOR_MODE 改成 ACTUATOR_RELAY，接线和引脚都不用动。 */
#define ACTUATOR_LED            1
#define ACTUATOR_RELAY          2
#define ACTUATOR_BUZZER         3

#define ACTUATOR_MODE           ACTUATOR_LED

#define ACTUATOR_PORT           GPIOB
#define ACTUATOR_PIN            GPIO_PIN_5

#if (ACTUATOR_MODE == ACTUATOR_RELAY) || (ACTUATOR_MODE == ACTUATOR_BUZZER)
#define ACTUATOR_ON_LEVEL       GPIO_PIN_RESET   /* 继电器 / 蜂鸣器模块：低电平触发 */
#else
#define ACTUATOR_ON_LEVEL       GPIO_PIN_SET     /* LED：高电平点亮 */
#endif
#define ACTUATOR_OFF_LEVEL      ((ACTUATOR_ON_LEVEL == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET)

/* ---------------------------- 板载 LED ---------------------------- */
#define LED_PORT                GPIOC
#define LED_PIN                 GPIO_PIN_13
#define LED_ON_LEVEL            GPIO_PIN_RESET
#define LED_OFF_LEVEL           ((LED_ON_LEVEL == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET)

/* ---------------------------- 业务参数 ---------------------------- */
#define APP_THRESHOLD_DEFAULT   30      /* 默认阈值 30% */
#define APP_THRESHOLD_STEP      5       /* 转一格调整 5% */
#define APP_THRESHOLD_MIN       5u
#define APP_THRESHOLD_MAX       95u
#define APP_HYSTERESIS          3       /* 回差 3%，防止输出频繁开关 */

#endif /* BOARD_H */
