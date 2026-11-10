/**
  ******************************************************************************
  * @file    board.h
  * @brief   板级引脚定义与硬件配置开关
  *
  * 接线（STM32F103C8T6 最小系统板）：
  *   DHT11 DATA  -> PA1   （模块自带 10k 上拉；裸传感器需外接 4.7k~10k 上拉到 3.3V）
  *   OLED SCL    -> PB6   （I2C1_SCL，模块 VCC 接 3.3V）
  *   OLED SDA    -> PB7   （I2C1_SDA）
  *   触摸模块 OUT -> PA2    （TTP223，默认高电平有效）
  *   蜂鸣器 IN   -> PB5    （低电平触发；高电平触发模块请改 BUZZER_ON_LEVEL）
  *   USB 串口    -> PA9/PA10（USART1，115200 8N1，调试用）
  ******************************************************************************
  */
#ifndef BOARD_H
#define BOARD_H

#include "main.h"

/* ---------------------------- 引脚分配 ---------------------------- */
#define DHT11_GPIO_PORT       GPIOA
#define DHT11_GPIO_PIN        GPIO_PIN_1

#define TOUCH_GPIO_PORT       GPIOA
#define TOUCH_GPIO_PIN        GPIO_PIN_2
#define TOUCH_ACTIVE_LEVEL    GPIO_PIN_SET     /* TTP223 默认触摸输出高电平 */

#define BUZZER_GPIO_PORT      GPIOB
#define BUZZER_GPIO_PIN       GPIO_PIN_5
#define BUZZER_ON_LEVEL       GPIO_PIN_RESET   /* 模块丝印：低电平触发；高电平模块改成 GPIO_PIN_SET */
/*
 * 注意：这里不能用 #if (BUZZER_ON_LEVEL == GPIO_PIN_SET) 判断——
 * GPIO_PIN_SET 是枚举常量不是宏，预处理器会把它当成 0，导致条件恒成立，
 * 上下两个电平全变成 RESET，蜂鸣器就会一直响。
 */
#define BUZZER_OFF_LEVEL      ((BUZZER_ON_LEVEL == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET)

/* 板载 LED：蓝色核心板 PC13，低电平点亮（用于上电自检与心跳指示） */
#define LED_GPIO_PORT         GPIOC
#define LED_GPIO_PIN          GPIO_PIN_13
#define LED_ON_LEVEL          GPIO_PIN_RESET
#define LED_OFF_LEVEL         ((LED_ON_LEVEL == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET)

/* ---------------------------- 功能参数 ---------------------------- */
#define APP_TEMP_HI_DEFAULT   30
#define APP_TEMP_LO_DEFAULT   10
#define APP_HUMI_HI_DEFAULT   80
#define APP_HUMI_LO_DEFAULT   30

#endif /* BOARD_H */
