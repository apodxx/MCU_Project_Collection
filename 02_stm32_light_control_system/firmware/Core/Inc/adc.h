/**
  ******************************************************************************
  * @file    adc.h
  * @brief   ADC1 单通道采集（PA0 光敏，PA1 电位器）
  ******************************************************************************
  */
#ifndef ADC_H
#define ADC_H

#include <stdint.h>

void     adc_init(void);
uint16_t adc_read_channel(uint32_t channel);
uint16_t adc_read_raw(void);

#endif /* ADC_H */
