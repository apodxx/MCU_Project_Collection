/**
  ******************************************************************************
  * @file    board.h
  * @brief   板级引脚定义与硬件开关（项目 03 · 声音检测与报警）
  *
  * 接线（STM32F103C8T6 最小系统板）：
  *   声音传感器 AO -> PA0    （ADC1_IN0，模拟量）
  *   旋转编码器 A   -> PA1
  *   旋转编码器 B   -> PA2
  *   旋转编码器 C   -> GND    （公共端）
  *   蜂鸣器 IN      -> PB5    （低电平触发；高电平模块改 BUZZER_ON_LEVEL）
  *   OLED SCL      -> PB10   （I2C2；接 PB6/I2C1 也能自动识别）
  *   OLED SDA      -> PB11
  *   调试串口       -> PA9/PA10（USART1，115200）
  ******************************************************************************
  */
#ifndef BOARD_H
#define BOARD_H

#include "main.h"

/* ---------------------------- 声音传感器 ---------------------------- */
#define SOUND_ADC_CHANNEL       ADC_CHANNEL_0   /* PA0 */

/* ---------------------------- 旋转编码器（360 度，A/C/B 三脚） ---------------------------- */
#define ENC_A_PORT              GPIOA
#define ENC_A_PIN               GPIO_PIN_1
#define ENC_B_PORT              GPIOA
#define ENC_B_PIN               GPIO_PIN_2

#define ENC_COUNTS_PER_STEP     4      /* 每格跳变数，转一格跳两下改成 8 */
#define ENC_REVERSE             0      /* 方向反了改 1，或对调 A/B */

/* ---------------------------- 蜂鸣器 ---------------------------- */
#define BUZZER_PORT             GPIOB
#define BUZZER_PIN              GPIO_PIN_5
#define BUZZER_ON_LEVEL         GPIO_PIN_RESET   /* 低电平触发；高电平模块改 GPIO_PIN_SET */
#define BUZZER_OFF_LEVEL        ((BUZZER_ON_LEVEL == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET)

/* ---------------------------- 板载 LED ---------------------------- */
#define LED_PORT                GPIOC
#define LED_PIN                 GPIO_PIN_13
#define LED_ON_LEVEL            GPIO_PIN_RESET
#define LED_OFF_LEVEL           ((LED_ON_LEVEL == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET)

/* ---------------------------- 业务参数 ---------------------------- */
#define APP_THRESHOLD_DEFAULT   60      /* 默认报警阈值 60% */
#define APP_THRESHOLD_STEP      5       /* 转一格调整 5% */
#define APP_THRESHOLD_MIN       5u
#define APP_THRESHOLD_MAX       95u
#define APP_ALARM_HYST          3       /* 回差 3%，防止报警抖动 */

#endif /* BOARD_H */
