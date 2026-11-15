/**
  ******************************************************************************
  * @file    oled_font.h
  * @brief   字库声明
  *
  * oled_font8x16 : 8x16 点阵 ASCII（0x20~0x7E）
  * oled_cn_table : 16x16 点阵中文字库子集，按 Unicode 码点查表
  *
  * 字库由 tools/gen_font.py 生成，新增中文时重新运行脚本即可。
  ******************************************************************************
  */
#ifndef OLED_FONT_H
#define OLED_FONT_H

#include <stdint.h>

#define OLED_ASCII_FIRST  0x20u
#define OLED_ASCII_LAST   0x7Eu

/* 每个 ASCII 字符 16 字节：每字节一行，高位在左 */
extern const uint8_t oled_font8x16[OLED_ASCII_LAST - OLED_ASCII_FIRST + 1][16];

typedef struct
{
  uint32_t       cp;       /* Unicode 码点 */
  const uint8_t *bitmap;   /* 32 字节：16 行 x 2 字节，高位在左 */
} oled_cn_glyph_t;

extern const oled_cn_glyph_t oled_cn_table[];
extern const uint16_t        oled_cn_count;

#endif /* OLED_FONT_H */
