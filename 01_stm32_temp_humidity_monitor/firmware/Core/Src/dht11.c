/**
  ******************************************************************************
  * @file    dht11.c
  * @brief   DHT11 驱动实现
  *
  * 时序要点（DHT11 数据手册）：
  *   主机拉低 >=18ms -> 释放总线 -> 传感器应答低 80us + 高 80us
  *   每个 bit：低 50us 起始 + 高电平 26~28us（0）或 70us（1）
  *
  * 微秒延时使用 Cortex-M3 的 DWT 周期计数器，精度 1/72us。
  ******************************************************************************
  */
#include "dht11.h"
#include "board.h"

#define DHT_TIMEOUT_US      150u

static void dht_delay_us(uint32_t us)
{
  uint32_t start = DWT->CYCCNT;
  uint32_t ticks = us * (SystemCoreClock / 1000000u);

  while ((DWT->CYCCNT - start) < ticks)
  {
  }
}

/* 等待总线到达指定电平，超时返回 -1 */
static int dht_wait_level(GPIO_PinState level)
{
  uint32_t t = 0;

  while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) != level)
  {
    if (++t > DHT_TIMEOUT_US)
    {
      return -1;
    }
    dht_delay_us(1);
  }
  return 0;
}

void dht11_init(void)
{
  GPIO_InitTypeDef gpio = {0};

  /* 开漏输出 + 内部上拉：写 1 即释放总线，读 IDR 得到实际电平 */
  gpio.Pin = DHT11_GPIO_PIN;
  gpio.Mode = GPIO_MODE_OUTPUT_OD;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(DHT11_GPIO_PORT, &gpio);
  HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_SET);

  /* 使能 DWT 周期计数器 */
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

int dht11_read(dht11_data_t *out)
{
  uint8_t data[5] = {0};
  uint32_t primask;
  int rc = DHT11_OK;
  uint8_t i;

  if (out == NULL)
  {
    return DHT11_ERR_PARAM;
  }

  /* 起始信号：拉低 20ms（使用 SysTick，不关闭中断） */
  HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_RESET);
  HAL_Delay(20);
  HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_SET);

  /* 位传输对时序敏感，这段关闭中断（约 3~5ms） */
  primask = __get_PRIMASK();
  __disable_irq();

  dht_delay_us(30);

  if (dht_wait_level(GPIO_PIN_RESET) != 0)   /* 传感器应答拉低 */
  {
    rc = DHT11_ERR_NO_RESPONSE;
  }
  else if (dht_wait_level(GPIO_PIN_SET) != 0) /* 应答低电平结束 */
  {
    rc = DHT11_ERR_NO_RESPONSE;
  }
  else if (dht_wait_level(GPIO_PIN_RESET) != 0) /* 应答高电平结束，进入数据位 */
  {
    rc = DHT11_ERR_NO_RESPONSE;
  }

  for (i = 0; (i < 40) && (rc == DHT11_OK); i++)
  {
    if (dht_wait_level(GPIO_PIN_SET) != 0)   /* 50us 低电平结束，高电平开始 */
    {
      rc = DHT11_ERR_TIMEOUT;
      break;
    }

    dht_delay_us(40);                        /* 在高电平中间采样 */
    data[i >> 3] <<= 1;
    if (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_SET)
    {
      data[i >> 3] |= 0x01;
    }

    if (dht_wait_level(GPIO_PIN_RESET) != 0) /* 等待当前 bit 的高电平结束 */
    {
      rc = DHT11_ERR_TIMEOUT;
      break;
    }
  }

  __set_PRIMASK(primask);

  if (rc != DHT11_OK)
  {
    return rc;
  }

  if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4])
  {
    return DHT11_ERR_CHECKSUM;
  }

  out->humidity = data[0];
  out->temperature = (int8_t)data[2];
  return DHT11_OK;
}
