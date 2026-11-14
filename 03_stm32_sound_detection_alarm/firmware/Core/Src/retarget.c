/**
  ******************************************************************************
  * @file    retarget.c
  * @brief   printf 重定向到 USART1（PB 无需额外配置，CubeMX 已初始化 115200 8N1）
  ******************************************************************************
  */
#include "usart.h"

int __io_putchar(int ch)
{
  uint8_t c = (uint8_t)ch;

  (void)HAL_UART_Transmit(&huart1, &c, 1u, 100u);
  return ch;
}
