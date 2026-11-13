/**
  ******************************************************************************
  * @file    encoder.h
  * @brief   EC11 旋转编码器（A/B 正交解码 + 按键）
  ******************************************************************************
  */
#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

void    encoder_init(void);
int8_t  encoder_poll(void);      /* 每格返回 +1 / -1，无动作返回 0 */
uint8_t encoder_pressed(void);   /* 1 = 按下 */

#endif /* ENCODER_H */
