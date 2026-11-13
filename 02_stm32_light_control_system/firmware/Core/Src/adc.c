/**
  ******************************************************************************
  * @file    adc.c
  * @brief   ADC1_IN0（PA0）单次转换，用于读取光敏电阻分压
  *
  * 说明：ADC 部分是手工初始化的（CubeMX 工程里只把 PA0 配成了模拟输入），
  *       因为 ADC1 的 IP 配置没有写进 .ioc，这里直接在代码里开时钟、校准。
  ******************************************************************************
  */
#include "adc.h"
#include "main.h"

ADC_HandleTypeDef hadc1;

void adc_init(void)
{
  ADC_ChannelConfTypeDef cfg = {0};

  __HAL_RCC_ADC1_CLK_ENABLE();

  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  cfg.Channel = ADC_CHANNEL_0;
  cfg.Rank = ADC_REGULAR_RANK_1;
  cfg.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &cfg) != HAL_OK)
  {
    Error_Handler();
  }

  (void)HAL_ADCEx_Calibration_Start(&hadc1);
}

uint16_t adc_read_raw(void)
{
  uint16_t value = 0;

  if (HAL_ADC_Start(&hadc1) == HAL_OK)
  {
    if (HAL_ADC_PollForConversion(&hadc1, 10u) == HAL_OK)
    {
      value = (uint16_t)HAL_ADC_GetValue(&hadc1);
    }
    (void)HAL_ADC_Stop(&hadc1);
  }
  return value;
}
