/**
  ******************************************************************************
  * @file    encoder.c
  * @brief   旋转编码器 A/B 正交解码（360 度模块，三脚 A/C/B）
  *
  * 原理：A/B 两相组成 2 位状态（00/01/11/10），按 Gray 码顺序变化。
  *       用一张 16 项状态表判断“上一个状态 -> 当前状态”是正转、反转还是抖动，
  *       累计到 ENC_COUNTS_PER_STEP 个有效跳变算一格。
  *
  * 接线：A -> PA1，B -> PA2，C（公共端）-> GND；A/B 用内部上拉。
  ******************************************************************************
  */
#include "encoder.h"
#include "board.h"

static const int8_t s_table[16] =
{
   0,  1, -1,  0,
  -1,  0,  0,  1,
   1,  0,  0, -1,
   0, -1,  1,  0
};

static uint8_t s_prev_state;
static int8_t  s_accum;

static uint8_t read_ab(void)
{
  uint8_t a = (HAL_GPIO_ReadPin(ENC_A_PORT, ENC_A_PIN) == GPIO_PIN_SET) ? 1u : 0u;
  uint8_t b = (HAL_GPIO_ReadPin(ENC_B_PORT, ENC_B_PIN) == GPIO_PIN_SET) ? 1u : 0u;

  return (uint8_t)((a << 1) | b);
}

void encoder_init(void)
{
  GPIO_InitTypeDef gpio = {0};

  /* 编码器是机械触点，另一端接 GND，这里用内部上拉 */
  gpio.Pin = ENC_A_PIN | ENC_B_PIN;
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(ENC_A_PORT, &gpio);

  s_prev_state = read_ab();
  s_accum = 0;
}

int8_t encoder_poll(void)
{
  uint8_t cur = read_ab();
  int8_t delta;

  if (cur == s_prev_state)
  {
    return 0;
  }

  delta = s_table[(uint8_t)((s_prev_state << 2) | cur)];
  s_prev_state = cur;

  if (delta == 0)
  {
    return 0;                    /* 无效跳变（抖动），忽略 */
  }

#if (ENC_REVERSE != 0)
  delta = (int8_t)(-delta);
#endif

  s_accum = (int8_t)(s_accum + delta);
  if (s_accum >= (int8_t)ENC_COUNTS_PER_STEP)
  {
    s_accum = 0;
    return 1;
  }
  if (s_accum <= -(int8_t)ENC_COUNTS_PER_STEP)
  {
    s_accum = 0;
    return -1;
  }
  return 0;
}
