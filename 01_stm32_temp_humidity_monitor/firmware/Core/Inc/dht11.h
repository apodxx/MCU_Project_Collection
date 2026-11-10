/**
  ******************************************************************************
  * @file    dht11.h
  * @brief   DHT11 温湿度传感器驱动（单总线，GPIO 模拟时序）
  ******************************************************************************
  */
#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>

/* 错误码 */
#define DHT11_OK              0
#define DHT11_ERR_PARAM      -1
#define DHT11_ERR_NO_RESPONSE -2   /* 传感器没有拉低总线应答 */
#define DHT11_ERR_TIMEOUT    -3    /* 位传输超时 */
#define DHT11_ERR_CHECKSUM   -4    /* 校验和错误 */

typedef struct
{
  int8_t  temperature;   /* 摄氏度，DHT11 分辨率为 1℃ */
  uint8_t humidity;      /* 相对湿度 %RH，分辨率 1% */
} dht11_data_t;

void dht11_init(void);
int  dht11_read(dht11_data_t *out);

#endif /* DHT11_H */
