/**
  ******************************************************************************
  * @file    oled.c
  * @brief   SSD1306 OLED 驱动实现
  *
  * 显存按页组织：8 页 x 128 列，每页 8 行像素。
  * 缓冲区每页首字节固定为 0x40（数据控制字节），后面 128 字节为像素数据，
  * 这样刷新时可以直接把整页发出去，省去一次内存拷贝。
  ******************************************************************************
  */
#include "oled.h"
#include "oled_font.h"
#include "i2c.h"
#include <string.h>

#define OLED_I2C_ADDR   (0x3C << 1)
#define OLED_CMD        0x00
#define OLED_DATA       0x40
#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_PAGES      (OLED_HEIGHT / 8)

#define OLED_I2C_TIMEOUT 100

static uint8_t s_buffer[OLED_PAGES][OLED_WIDTH + 1];
static uint8_t s_ready;
static uint8_t s_error_count;

static const uint8_t s_init_cmds[] =
{
  0xAE,             /* 关闭显示 */
  0x20, 0x02,       /* 页地址模式 */
  0xB0,             /* 页起始地址 */
  0xC8,             /* COM 扫描方向：从下到上 */
  0x00, 0x10,       /* 列地址低位/高位 */
  0x40,             /* 显示起始行 */
  0x81, 0xCF,       /* 对比度 */
  0xA1,             /* 段重映射：左右翻转 */
  0xA6,             /* 正常显示 */
  0xA8, 0x3F,       /* 多路复用 1/64 */
  0xA4,             /* 输出跟随显存 */
  0xD3, 0x00,       /* 显示偏移 */
  0xD5, 0x80,       /* 时钟分频 */
  0xD9, 0xF1,       /* 预充电周期 */
  0xDA, 0x12,       /* COM 引脚配置 */
  0xDB, 0x40,       /* VCOMH */
  0x8D, 0x14,       /* 电荷泵使能 */
  0xAF              /* 开启显示 */
};

static void oled_set_ready(uint8_t ready)
{
  s_ready = ready;
  if (ready)
  {
    s_error_count = 0;
  }
}

uint8_t oled_ready(void)
{
  return s_ready;
}

uint8_t oled_init(void)
{
  uint8_t page;

  for (page = 0; page < OLED_PAGES; page++)
  {
    s_buffer[page][0] = OLED_DATA;
  }
  memset(&s_buffer[0][1], 0x00, OLED_WIDTH);
  for (page = 1; page < OLED_PAGES; page++)
  {
    memset(&s_buffer[page][1], 0x00, OLED_WIDTH);
  }

  if (HAL_I2C_Mem_Write(&hi2c1, OLED_I2C_ADDR, OLED_CMD, I2C_MEMADD_SIZE_8BIT,
                        (uint8_t *)s_init_cmds, sizeof(s_init_cmds),
                        OLED_I2C_TIMEOUT) != HAL_OK)
  {
    oled_set_ready(0);
    return 0;
  }

  oled_set_ready(1);
  oled_flush();
  return 1;
}

void oled_clear(void)
{
  uint8_t page;

  for (page = 0; page < OLED_PAGES; page++)
  {
    memset(&s_buffer[page][1], 0x00, OLED_WIDTH);
  }
}

void oled_flush(void)
{
  uint8_t page;
  uint8_t cmds[3];

  if (!s_ready)
  {
    return;
  }

  for (page = 0; page < OLED_PAGES; page++)
  {
    cmds[0] = (uint8_t)(0xB0 | page);
    cmds[1] = 0x00;
    cmds[2] = 0x10;

    if ((HAL_I2C_Mem_Write(&hi2c1, OLED_I2C_ADDR, OLED_CMD, I2C_MEMADD_SIZE_8BIT,
                           cmds, sizeof(cmds), OLED_I2C_TIMEOUT) != HAL_OK) ||
        (HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDR, s_buffer[page],
                                 OLED_WIDTH + 1, OLED_I2C_TIMEOUT) != HAL_OK))
    {
      if (++s_error_count >= 5)
      {
        oled_set_ready(0);   /* 连续失败，交给上层重新初始化 */
      }
      return;
    }
  }
  s_error_count = 0;
}

void oled_pixel(uint8_t x, uint8_t y, uint8_t on)
{
  uint8_t mask;

  if ((x >= OLED_WIDTH) || (y >= OLED_HEIGHT))
  {
    return;
  }

  mask = (uint8_t)(1u << (y & 0x07));
  if (on)
  {
    s_buffer[y >> 3][x + 1] |= mask;
  }
  else
  {
    s_buffer[y >> 3][x + 1] &= (uint8_t)~mask;
  }
}

void oled_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t on)
{
  uint16_t i;
  uint16_t j;

  for (j = 0; j < h; j++)
  {
    for (i = 0; i < w; i++)
    {
      oled_pixel((uint8_t)(x + i), (uint8_t)(y + j), on);
    }
  }
}

static void oled_draw_ascii(uint8_t x, uint8_t y, uint8_t ch)
{
  const uint8_t *glyph;
  uint8_t row;
  uint8_t col;

  if ((ch < OLED_ASCII_FIRST) || (ch > OLED_ASCII_LAST))
  {
    ch = '?';
  }
  glyph = oled_font8x16[ch - OLED_ASCII_FIRST];

  for (row = 0; row < 16; row++)
  {
    uint8_t bits = glyph[row];
    for (col = 0; col < 8; col++)
    {
      if (bits & (uint8_t)(0x80u >> col))
      {
        oled_pixel((uint8_t)(x + col), (uint8_t)(y + row), 1);
      }
    }
  }
}

static const uint8_t *oled_find_cn(uint32_t cp)
{
  uint16_t i;

  for (i = 0; i < oled_cn_count; i++)
  {
    if (oled_cn_table[i].cp == cp)
    {
      return oled_cn_table[i].bitmap;
    }
  }
  return NULL;
}

static void oled_draw_cn(uint8_t x, uint8_t y, const uint8_t *glyph)
{
  uint8_t row;
  uint8_t col;

  for (row = 0; row < 16; row++)
  {
    uint16_t bits = (uint16_t)(((uint16_t)glyph[row * 2] << 8) | glyph[row * 2 + 1]);
    for (col = 0; col < 16; col++)
    {
      if (bits & (uint16_t)(0x8000u >> col))
      {
        oled_pixel((uint8_t)(x + col), (uint8_t)(y + row), 1);
      }
    }
  }
}

/* 解析一个 UTF-8 字符，返回码点并前移指针 */
static uint32_t oled_utf8_next(const char **pp)
{
  const uint8_t *p = (const uint8_t *)(*pp);
  uint32_t cp;

  if (p[0] < 0x80u)
  {
    cp = p[0];
    *pp += 1;
  }
  else if ((p[0] & 0xE0u) == 0xC0u)
  {
    cp = (uint32_t)(((p[0] & 0x1Fu) << 6) | (p[1] & 0x3Fu));
    *pp += 2;
  }
  else if ((p[0] & 0xF0u) == 0xE0u)
  {
    cp = (uint32_t)(((p[0] & 0x0Fu) << 12) |
                    ((p[1] & 0x3Fu) << 6) |
                    (p[2] & 0x3Fu));
    *pp += 3;
  }
  else
  {
    cp = 0xFFFDu;
    *pp += 1;
  }
  return cp;
}

uint16_t oled_text_width(const char *text)
{
  uint16_t width = 0;
  uint32_t cp;

  while (*text != '\0')
  {
    cp = oled_utf8_next(&text);
    width += (cp < 0x80u) ? 8u : 16u;
  }
  return width;
}

void oled_draw_text(uint8_t x, uint8_t y, const char *text)
{
  uint32_t cp;

  while (*text != '\0')
  {
    cp = oled_utf8_next(&text);
    if (cp < 0x80u)
    {
      oled_draw_ascii(x, y, (uint8_t)cp);
      x = (uint8_t)(x + 8);
    }
    else
    {
      const uint8_t *glyph = oled_find_cn(cp);
      if (glyph != NULL)
      {
        oled_draw_cn(x, y, glyph);
      }
      else
      {
        oled_draw_ascii(x, y, '?');
      }
      x = (uint8_t)(x + 16);
    }
  }
}

void oled_draw_text_center(uint8_t y, const char *text)
{
  uint16_t width = oled_text_width(text);

  if (width >= OLED_WIDTH)
  {
    oled_draw_text(0, y, text);
  }
  else
  {
    oled_draw_text((uint8_t)((OLED_WIDTH - width) / 2), y, text);
  }
}
