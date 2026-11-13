/**
  ******************************************************************************
  * @file    adc.h
  * @brief   ADC1 单通道采集（PA0，光敏电阻）
  ******************************************************************************
  */
#ifndef ADC_H
#define ADC_H

#include <stdint.h>

void     adc_init(void);
uint16_t adc_read_raw(void);

#endif /* ADC_H */
