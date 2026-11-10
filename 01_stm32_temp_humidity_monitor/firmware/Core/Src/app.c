/**
  ******************************************************************************
  * @file    app.c
  * @brief   应用逻辑：采集温湿度、上下限判断、报警、按键交互与界面显示
  *
  * 界面共 5 页：主页 / 温度上限 / 温度下限 / 湿度上限 / 湿度下限
  * 操作（单触摸模块）：
  *   短按          主页之外：翻到下一页
  *   长按（>=0.8s） 设置页：进入 / 退出编辑
  *   编辑时短按     数值 +1（湿度 +5），到上限后回到下限
  ******************************************************************************
  */
#include "app.h"
#include "board.h"
#include "dht11.h"
#include "i2c.h"
#include "oled.h"
#include <stdio.h>
#include <string.h>

#define READ_PERIOD_MS       2000u   /* DHT11 采集周期，手册建议 >=1s */
#define SENSOR_TIMEOUT_MS    6000u   /* 超过该时间没有有效数据则判定传感器异常 */
#define TOUCH_DEBOUNCE_MS    20u
#define TOUCH_LONG_MS        800u
#define UI_PERIOD_MS         200u
#define BLINK_PERIOD_MS      400u
#define OLED_RETRY_MS        1000u   /* OLED 初始化失败后的重试间隔 */
#define BEEP_ON_MS           200u
#define BEEP_PERIOD_MS       1000u
#define POWER_UP_DELAY_MS    1200u   /* 上电后等待 DHT11 稳定 */
#define ALARM_COUNT_MAX      99999u

typedef enum
{
  PAGE_MAIN = 0,
  PAGE_T_HI,
  PAGE_T_LO,
  PAGE_H_HI,
  PAGE_H_LO,
  PAGE_MAX
} page_t;

static const char *const s_titles[PAGE_MAX] =
{
  "", "温度上限", "温度下限", "湿度上限", "湿度下限"
};

static const char *const s_units[PAGE_MAX] =
{
  "", "℃", "℃", "%", "%"
};

static page_t s_page;
static uint8_t s_editing;

static int16_t s_t_hi;
static int16_t s_t_lo;
static int16_t s_h_hi;
static int16_t s_h_lo;

static dht11_data_t s_dht;
static uint8_t  s_dht_valid;
static uint32_t s_last_ok_tick;
static uint32_t s_last_read_tick;

static uint8_t  s_alarm;
static uint32_t s_alarm_count;

static uint8_t  s_touch_level;
static uint8_t  s_touch_raw;
static uint32_t s_touch_raw_tick;
static uint32_t s_touch_press_tick;
static uint8_t  s_touch_long_fired;

static uint8_t  s_dirty;
static uint32_t s_last_render_tick;
static uint32_t s_oled_retry_tick;

/* ------------------------------ 基础工具 ------------------------------ */

static void buzzer_set(uint8_t on)
{
  HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN,
                    (on != 0u) ? BUZZER_ON_LEVEL : BUZZER_OFF_LEVEL);
}

static void led_set(uint8_t on)
{
  HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN,
                    (on != 0u) ? LED_ON_LEVEL : LED_OFF_LEVEL);
}

static void led_init(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  gpio.Pin = LED_GPIO_PIN;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_PORT, &gpio);
  led_set(0);
}

/* 开机自检提示音：n 声短鸣（1 声=正常，2 声=OLED 没找到） */
static void beep_times(uint8_t times)
{
  uint8_t i;

  for (i = 0; i < times; i++)
  {
    buzzer_set(1);
    HAL_Delay(80);
    buzzer_set(0);
    HAL_Delay(140);
  }
}

static void i2c_scan_report(void)
{
  uint16_t addr;

  printf("[I2C] devices:");
  for (addr = 0x08u; addr < 0x78u; addr++)
  {
    if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(addr << 1), 2u, 20u) == HAL_OK)
    {
      printf(" 0x%02X", (unsigned int)addr);
    }
  }
  printf("\r\n");
}

static uint8_t touch_pressed(void)
{
  return (HAL_GPIO_ReadPin(TOUCH_GPIO_PORT, TOUCH_GPIO_PIN) == TOUCH_ACTIVE_LEVEL) ? 1u : 0u;
}

static uint8_t sensor_ok(void)
{
  if (!s_dht_valid)
  {
    return 0;
  }
  return ((HAL_GetTick() - s_last_ok_tick) < SENSOR_TIMEOUT_MS) ? 1u : 0u;
}

/* ------------------------------ 阈值管理 ------------------------------ */

static int16_t *page_value(page_t page)
{
  switch (page)
  {
    case PAGE_T_HI: return &s_t_hi;
    case PAGE_T_LO: return &s_t_lo;
    case PAGE_H_HI: return &s_h_hi;
    case PAGE_H_LO: return &s_h_lo;
    default:        return NULL;
  }
}

static int16_t page_step(page_t page)
{
  return (page == PAGE_H_HI || page == PAGE_H_LO) ? 5 : 1;
}

static void page_bounds(page_t page, int16_t *lo, int16_t *hi)
{
  switch (page)
  {
    case PAGE_T_HI:
      *lo = (int16_t)(s_t_lo + 1);
      if (*lo < 15) { *lo = 15; }
      *hi = 45;
      break;
    case PAGE_T_LO:
      *lo = 0;
      *hi = (int16_t)(s_t_hi - 1);
      if (*hi > 25) { *hi = 25; }
      break;
    case PAGE_H_HI:
      *lo = (int16_t)(s_h_lo + 5);
      if (*lo < 50) { *lo = 50; }
      *hi = 95;
      break;
    case PAGE_H_LO:
      *lo = 10;
      *hi = (int16_t)(s_h_hi - 5);
      if (*hi > 50) { *hi = 50; }
      break;
    default:
      *lo = 0;
      *hi = 0;
      break;
  }
}

static void setting_increment(void)
{
  int16_t lo;
  int16_t hi;
  int16_t *value = page_value(s_page);

  if (value == NULL)
  {
    return;
  }

  page_bounds(s_page, &lo, &hi);
  *value = (int16_t)(*value + page_step(s_page));
  if (*value > hi)
  {
    *value = lo;
  }
}

/* ------------------------------ 按键处理 ------------------------------ */

static void on_short_press(void)
{
  if (s_page == PAGE_MAIN)
  {
    s_page = PAGE_T_HI;
  }
  else if (s_editing)
  {
    setting_increment();
    printf("[SET] %s = %d\r\n", (s_page == PAGE_T_HI) ? "T_HI" :
                                (s_page == PAGE_T_LO) ? "T_LO" :
                                (s_page == PAGE_H_HI) ? "H_HI" : "H_LO",
           (int)*page_value(s_page));
  }
  else
  {
    s_page = (page_t)((s_page + 1) % PAGE_MAX);
  }
  s_dirty = 1;
}

static void on_long_press(void)
{
  if (s_page == PAGE_MAIN)
  {
    return;                     /* 主页长按暂不定义 */
  }

  s_editing = (uint8_t)((s_editing == 0u) ? 1u : 0u);
  s_dirty = 1;
  printf("[SET] edit %s\r\n", (s_editing != 0u) ? "on" : "off");
}

static void button_poll(void)
{
  uint32_t now = HAL_GetTick();
  uint8_t raw = touch_pressed();

  if (raw != s_touch_raw)
  {
    s_touch_raw = raw;
    s_touch_raw_tick = now;
  }

  if (((now - s_touch_raw_tick) >= TOUCH_DEBOUNCE_MS) && (raw != s_touch_level))
  {
    s_touch_level = raw;
    if (s_touch_level != 0u)
    {
      s_touch_press_tick = now;
      s_touch_long_fired = 0;
    }
    else if ((s_touch_long_fired == 0u) && ((now - s_touch_press_tick) < TOUCH_LONG_MS))
    {
      on_short_press();
    }
    else
    {
      /* 长按已在按下期间处理，松开不做动作 */
    }
  }

  if ((s_touch_level != 0u) && (s_touch_long_fired == 0u) &&
      ((now - s_touch_press_tick) >= TOUCH_LONG_MS))
  {
    s_touch_long_fired = 1;
    on_long_press();
  }
}

/* ------------------------------ 采集与报警 ------------------------------ */

static void sensor_poll(void)
{
  uint32_t now = HAL_GetTick();
  dht11_data_t data;
  int rc;

  if ((now < POWER_UP_DELAY_MS) || ((now - s_last_read_tick) < READ_PERIOD_MS))
  {
    return;
  }
  s_last_read_tick = now;

  rc = dht11_read(&data);
  if (rc == DHT11_OK)
  {
    s_dht = data;
    s_dht_valid = 1;
    s_last_ok_tick = now;
    printf("[DHT11] T=%dC H=%d%%\r\n", (int)data.temperature, (int)data.humidity);
  }
  else
  {
    printf("[DHT11] read failed, rc=%d\r\n", rc);
  }
  s_dirty = 1;
}

static void alarm_manage(void)
{
  uint8_t alarm = 0;

  if (sensor_ok())
  {
    alarm = ((s_dht.temperature > s_t_hi) ||
             (s_dht.temperature < s_t_lo) ||
             (s_dht.humidity > s_h_hi) ||
             (s_dht.humidity < s_h_lo)) ? 1u : 0u;
  }

  if ((alarm != 0u) && (s_alarm == 0u))
  {
    if (s_alarm_count < ALARM_COUNT_MAX)
    {
      s_alarm_count++;
    }
    printf("[ALARM] count=%u\r\n", (unsigned int)s_alarm_count);
    s_dirty = 1;
  }
  s_alarm = alarm;

  if (alarm != 0u)
  {
    buzzer_set(((HAL_GetTick() % BEEP_PERIOD_MS) < BEEP_ON_MS) ? 1u : 0u);
  }
  else
  {
    buzzer_set(0);
  }
}

/* ------------------------------ 界面显示 ------------------------------ */

static void render_main(uint8_t blink_on)
{
  char buf[20];
  uint8_t ok = sensor_ok();

  oled_draw_text(0, 0, "温度");
  if (ok != 0u)
  {
    (void)snprintf(buf, sizeof(buf), "%d℃", (int)s_dht.temperature);
  }
  else
  {
    (void)snprintf(buf, sizeof(buf), "--");
  }
  oled_draw_text(40, 0, buf);

  oled_draw_text(0, 16, "湿度");
  if (ok != 0u)
  {
    (void)snprintf(buf, sizeof(buf), "%d%%", (int)s_dht.humidity);
  }
  else
  {
    (void)snprintf(buf, sizeof(buf), "--");
  }
  oled_draw_text(40, 16, buf);

  oled_draw_text(0, 32, "状态");
  if (ok == 0u)
  {
    if (blink_on != 0u)
    {
      oled_draw_text(40, 32, "异常");
    }
  }
  else if (s_alarm != 0u)
  {
    if (blink_on != 0u)
    {
      oled_draw_text(40, 32, "超限");
    }
  }
  else
  {
    oled_draw_text(40, 32, "正常");
  }

  oled_draw_text(0, 48, "报警");
  if (ok == 0u)
  {
    oled_draw_text(40, 48, "检查接线");
  }
  else
  {
    (void)snprintf(buf, sizeof(buf), "%u次", (unsigned int)s_alarm_count);
    oled_draw_text(40, 48, buf);
  }
}

static void render_setting(void)
{
  char buf[20];
  int16_t value = *page_value(s_page);

  oled_draw_text_center(0, s_titles[s_page]);

  if (s_editing != 0u)
  {
    (void)snprintf(buf, sizeof(buf), "[%d%s]", (int)value, s_units[s_page]);
  }
  else
  {
    (void)snprintf(buf, sizeof(buf), "%d%s", (int)value, s_units[s_page]);
  }
  oled_draw_text_center(16, buf);

  oled_draw_text_center(32, (s_editing != 0u) ? "短按 加一" : "短按 翻页");
  oled_draw_text_center(48, (s_editing != 0u) ? "长按 完成" : "长按 编辑");
}

static void render(void)
{
  uint8_t blink_on = (uint8_t)((HAL_GetTick() / BLINK_PERIOD_MS) & 0x01u);

  oled_clear();
  if (s_page == PAGE_MAIN)
  {
    render_main(blink_on);
  }
  else
  {
    render_setting();
  }
  oled_flush();
}

static void ui_task(void)
{
  uint32_t now = HAL_GetTick();

  if (oled_ready() == 0u)
  {
    if ((now - s_oled_retry_tick) >= OLED_RETRY_MS)
    {
      s_oled_retry_tick = now;
      if (oled_init() != 0u)
      {
        s_dirty = 1;
      }
    }
    return;                     /* OLED 未接好时继续跑采集与报警 */
  }

  if ((s_dirty == 0u) && ((now - s_last_render_tick) < UI_PERIOD_MS))
  {
    return;
  }

  render();
  s_dirty = 0;
  s_last_render_tick = now;
}

/* ------------------------------ 对外接口 ------------------------------ */

void app_init(void)
{
  GPIO_InitTypeDef gpio = {0};

  (void)setvbuf(stdout, NULL, _IONBF, 0);

  buzzer_set(0);
  led_init();

  /* 上电自检：LED 快闪 3 次 —— 只要程序跑起来，一定能看到 */
  for (uint8_t i = 0; i < 3u; i++)
  {
    led_set(1);
    HAL_Delay(100);
    led_set(0);
    HAL_Delay(100);
  }

  /* 触摸模块输出：下拉输入，避免悬空时误触发 */
  gpio.Pin = TOUCH_GPIO_PIN;
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_PULLDOWN;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TOUCH_GPIO_PORT, &gpio);

  dht11_init();

  s_t_hi = APP_TEMP_HI_DEFAULT;
  s_t_lo = APP_TEMP_LO_DEFAULT;
  s_h_hi = APP_HUMI_HI_DEFAULT;
  s_h_lo = APP_HUMI_LO_DEFAULT;

  s_page = PAGE_MAIN;
  s_editing = 0;
  s_dirty = 1;
  s_last_read_tick = HAL_GetTick() - READ_PERIOD_MS;

  printf("=== Project 01: STM32 Temp & Humidity Monitor ===\r\n");
  printf("[SYS] HCLK=%lu Hz, SYSCLK=%lu Hz\r\n",
         (unsigned long)HAL_RCC_GetHCLKFreq(),
         (unsigned long)HAL_RCC_GetSysClockFreq());

  i2c_scan_report();

  if (oled_init() != 0u)
  {
    printf("[OLED] ok, addr=0x%02X\r\n", (unsigned int)oled_i2c_addr());
    beep_times(1);              /* 一声：程序正常、屏幕已识别 */
  }
  else
  {
    printf("[OLED] not found! check VCC=3V3 GND SCL=PB6 SDA=PB7\r\n");
    beep_times(2);              /* 两声：程序在跑，但屏幕没找到 */
  }

  printf("T_HI=%d T_LO=%d H_HI=%d H_LO=%d\r\n",
         (int)s_t_hi, (int)s_t_lo, (int)s_h_hi, (int)s_h_lo);
}

void app_task(void)
{
  /* 心跳灯：每秒亮灭各 500ms，说明主循环没卡死 */
  led_set((HAL_GetTick() % 1000u) < 500u ? 1u : 0u);

  button_poll();
  sensor_poll();
  alarm_manage();
  ui_task();

  HAL_Delay(2);   /* 主循环降频，减少无谓的总线/GPIO 操作 */
}
