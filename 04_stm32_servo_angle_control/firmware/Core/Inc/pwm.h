/**
  ******************************************************************************
  * @file    pwm.h
  * @brief   舵机 PWM（TIM2_CH1 / PA0，50Hz）
  ******************************************************************************
  */
#ifndef PWM_H
#define PWM_H

#include <stdint.h>

void pwm_init(void);
void servo_set_angle(uint8_t angle);

#endif /* PWM_H */
