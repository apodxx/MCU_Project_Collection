/**
  ******************************************************************************
  * @file    board.h
  * @brief   板级引脚定义与硬件开关（项目 04 · 自动闸门）
  *
  * 接线（STM32F103C8T6 最小系统板）：
  *   舵机信号（橙线） -> PA0    （TIM2_CH1，50Hz PWM）
  *   旋转编码器 A     -> PA1
  *   旋转编码器 B     -> PA2
  *   旋转编码器 C     -> GND    （公共端）
  *   触摸模块 OUT     -> PA3    （手动模式开/关闸）
  *   红外避障 OUT     -> PA4    （检测车辆，低电平有效）
  *   蜂鸣器 IN        -> PB5    （低电平触发）
  *   OLED SCL        -> PB10   （I2C2；接 PB6/I2C1 也能自动识别）
  *   OLED SDA        -> PB11
  *   调试串口         -> PA9/PA10（USART1，115200）
  ******************************************************************************
  */
#ifndef BOARD_H
#define BOARD_H

#include "main.h"

/* ---------------------------- 舵机 PWM ---------------------------- */
#define SERVO_TIM_CHANNEL       TIM_CHANNEL_1   /* PA0 = TIM2_CH1 */
#define SERVO_PULSE_MIN_US      500u            /* 0° 对应 0.5ms */
#define SERVO_PULSE_MAX_US      2500u           /* 180° 对应 2.5ms */
#define SERVO_ANGLE_MIN         0u
#define SERVO_ANGLE_MAX         180u

/* ---------------------------- 旋转编码器（切模式） ---------------------------- */
#define ENC_A_PORT              GPIOA
#define ENC_A_PIN               GPIO_PIN_1
#define ENC_B_PORT              GPIOA
#define ENC_B_PIN               GPIO_PIN_2

#define ENC_COUNTS_PER_STEP     4
#define ENC_REVERSE             0

/* ---------------------------- 触摸按键 ---------------------------- */
#define TOUCH_PORT              GPIOA
#define TOUCH_PIN               GPIO_PIN_3
#define TOUCH_ACTIVE_LEVEL      GPIO_PIN_SET

/* ---------------------------- 红外避障（车辆检测） ---------------------------- */
#define IR_PORT                 GPIOA
#define IR_PIN                  GPIO_PIN_4
/* 大多数红外避障模块：检测到物体输出低电平；如果你的模块相反，改成 GPIO_PIN_SET */
#define IR_ACTIVE_LEVEL         GPIO_PIN_RESET

/* ---------------------------- 蜂鸣器 ---------------------------- */
#define BUZZER_PORT             GPIOB
#define BUZZER_PIN              GPIO_PIN_5
#define BUZZER_ON_LEVEL         GPIO_PIN_RESET
#define BUZZER_OFF_LEVEL        ((BUZZER_ON_LEVEL == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET)

/* ---------------------------- 板载 LED ---------------------------- */
#define LED_PORT                GPIOC
#define LED_PIN                 GPIO_PIN_13
#define LED_ON_LEVEL            GPIO_PIN_RESET
#define LED_OFF_LEVEL           ((LED_ON_LEVEL == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET)

/* ---------------------------- 业务参数 ---------------------------- */
#define APP_GATE_OPEN_ANGLE     90      /* 开闸角度：栏杆竖起 */
#define APP_GATE_CLOSE_ANGLE    0       /* 关闸角度：栏杆水平 */
#define APP_AUTO_CLOSE_MS       3000    /* 车走后多久自动关闸 */
#define APP_IR_DEBOUNCE_MS      50      /* 红外消抖 */
#define APP_RAMP_MS             20      /* 每 20ms 走一步 */
#define APP_RAMP_STEP           2       /* 每步 2° */
#define APP_ARRIVE_TOL          1       /* 差 1° 以内算到位 */
#define APP_BEEP_ARRIVE_MS      60      /* 到位提示音 */
#define APP_BEEP_CAR_MS         300     /* 手动模式“有车”提醒音 */
#define APP_BEEP_LIMIT_MS       200     /* 手动关闸提示音 */

#endif /* BOARD_H */
