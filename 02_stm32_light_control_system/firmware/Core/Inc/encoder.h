/**
  ******************************************************************************
  * @file    encoder.h
  * @brief   旋转编码器（A/B 正交解码，360 度，无按键）
  ******************************************************************************
  */
#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

void    encoder_init(void);
int8_t  encoder_poll(void);      /* 每格返回 +1 / -1，无动作返回 0 */

#endif /* ENCODER_H */
