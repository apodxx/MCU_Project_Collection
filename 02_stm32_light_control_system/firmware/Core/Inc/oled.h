/**
  ******************************************************************************
  * @file    oled.h
  * @brief   SSD1306 0.96 寸 OLED（128x64，I2C）驱动
  ******************************************************************************
  */
#ifndef OLED_H
#define OLED_H

#include <stdint.h>

uint8_t  oled_init(void);          /* 返回 1 表示初始化成功 */
uint8_t  oled_ready(void);
uint8_t  oled_i2c_addr(void);      /* 当前使用的 7 位 I2C 地址 */
const char *oled_bus_name(void);   /* "I2C1" 或 "I2C2" */
void     oled_clear(void);
void     oled_flush(void);
void     oled_pixel(uint8_t x, uint8_t y, uint8_t on);
void     oled_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t on);
void     oled_draw_text(uint8_t x, uint8_t y, const char *text);
uint16_t oled_text_width(const char *text);
void     oled_draw_text_center(uint8_t y, const char *text);

#endif /* OLED_H */
