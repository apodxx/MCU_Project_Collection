/**
  ******************************************************************************
  * @file    pwm.c
  * @brief   TIM2_CH1 输出 50Hz PWM 驱动 SG90
  *
  * 时钟：72MHz / 72 = 1MHz（1 个计数 = 1µs）
  * 周期：20000 计数 = 20ms（50Hz）
  * 脉宽：0.5ms ~ 2.5ms 对应 0° ~ 180°
  *
  * 说明：TIM2 是手工初始化的（.ioc 里 PA0 先配成普通输出，这里改成复用推挽），
  *       所以 CubeMX 重新生成代码不会覆盖这段逻辑。
  ******************************************************************************
  */
#include "pwm.h"
#include "board.h"

TIM_HandleTypeDef htim2;

void pwm_init(void)
{
  GPIO_InitTypeDef gpio = {0};
  TIM_OC_InitTypeDef oc = {0};

  __HAL_RCC_TIM2_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* PA0 -> TIM2_CH1 复用推挽输出 */
  gpio.Pin = GPIO_PIN_0;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &gpio);

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 72u - 1u;          /* 72MHz / 72 = 1MHz */
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 20000u - 1u;          /* 20ms 周期 */
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }

  oc.OCMode = TIM_OCMODE_PWM1;
  oc.Pulse = SERVO_PULSE_MIN_US;            /* 上电先给 0°，避免舵机乱甩 */
  oc.OCPolarity = TIM_OCPOLARITY_HIGH;
  oc.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &oc, SERVO_TIM_CHANNEL) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_PWM_Start(&htim2, SERVO_TIM_CHANNEL) != HAL_OK)
  {
    Error_Handler();
  }
}

void servo_set_angle(uint8_t angle)
{
  uint32_t pulse;

  if (angle > SERVO_ANGLE_MAX)
  {
    angle = SERVO_ANGLE_MAX;
  }

  pulse = SERVO_PULSE_MIN_US +
          ((uint32_t)angle * (SERVO_PULSE_MAX_US - SERVO_PULSE_MIN_US)) / SERVO_ANGLE_MAX;
  __HAL_TIM_SET_COMPARE(&htim2, SERVO_TIM_CHANNEL, pulse);
}
