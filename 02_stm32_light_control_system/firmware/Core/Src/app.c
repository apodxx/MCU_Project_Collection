/**
  ******************************************************************************
  * @file    app.c
  * @brief   智能光照控制：ADC 采光、阈值判断、继电器控制、编码器交互
  *
  * 交互：
  *   主界面   短按 = 自动/手动切换     长按 = 进入阈值设置
  *            自动模式旋转 = 调阈值     手动模式旋转 = 开灯 / 关灯
  *   设置页   旋转 = 调阈值             短按 = 返回主界面
  ******************************************************************************
  */
#include "app.h"
#include "adc.h"
#include "board.h"
#include "encoder.h"
#include "i2c.h"
#include "oled.h"
#include <stdio.h>

#define SAMPLE_PERIOD_MS   100u
#define UI_PERIOD_MS       200u
#define HINT_PERIOD_MS     3000u
#define OLED_RETRY_MS      1000u
#define BTN_DEBOUNCE_MS    20u
#define BTN_LONG_MS        800u

#define THRESHOLD_MIN      5u
#define THRESHOLD_MAX      95u

typedef enum
{
  PAGE_MAIN = 0,
  PAGE_SET
} page_t;

typedef enum
{
  MODE_AUTO = 0,
  MODE_MANUAL
} mode_t;

static page_t   s_page;
static mode_t   s_mode;
static uint8_t  s_threshold;
static uint8_t  s_light;
static uint8_t  s_light_valid;
static uint8_t  s_lamp;

static uint32_t s_last_sample;
static uint8_t  s_dirty;
static uint32_t s_last_render;
static uint32_t s_oled_retry;

static uint8_t  s_btn_level;
static uint8_t  s_btn_raw;
static uint32_t s_btn_raw_tick;
static uint32_t s_btn_press_tick;
static uint8_t  s_btn_long_fired;

/* ------------------------------ 基础工具 ------------------------------ */

static void led_set(uint8_t on)
{
  HAL_GPIO_WritePin(LED_PORT, LED_PIN, (on != 0u) ? LED_ON_LEVEL : LED_OFF_LEVEL);
}

static void led_init(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  gpio.Pin = LED_PIN;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_PORT, &gpio);
  led_set(0);
}

static void relay_write(uint8_t on)
{
  HAL_GPIO_WritePin(RELAY_PORT, RELAY_PIN, (on != 0u) ? RELAY_ON_LEVEL : RELAY_OFF_LEVEL);
}

static void set_lamp(uint8_t on)
{
  if (on == s_lamp)
  {
    return;
  }
  s_lamp = on;
  relay_write(on);
  s_dirty = 1;
  printf("[RELAY] %s\r\n", (on != 0u) ? "ON" : "OFF");
}

/* 自动模式：低于阈值开灯，高于“阈值 + 回差”关灯 */
static void apply_auto_control(void)
{
  if ((s_mode != MODE_AUTO) || (s_light_valid == 0u))
  {
    return;
  }
  if (s_light <= s_threshold)
  {
    set_lamp(1);
  }
  else if (s_light >= (uint8_t)(s_threshold + APP_HYSTERESIS))
  {
    set_lamp(0);
  }
  else
  {
    /* 回差区间内保持当前状态，避免继电器反复吸合 */
  }
}

/* ------------------------------ 参数调整 ------------------------------ */

static void adjust_threshold(int8_t dir)
{
  int16_t value = (int16_t)s_threshold + ((int16_t)dir * (int16_t)APP_THRESHOLD_STEP);

  if (value < (int16_t)THRESHOLD_MIN)
  {
    value = (int16_t)THRESHOLD_MIN;
  }
  if (value > (int16_t)THRESHOLD_MAX)
  {
    value = (int16_t)THRESHOLD_MAX;
  }

  if ((uint8_t)value != s_threshold)
  {
    s_threshold = (uint8_t)value;
    s_dirty = 1;
    printf("[TH] %u%%\r\n", (unsigned int)s_threshold);
    apply_auto_control();
  }
}

/* ------------------------------ 按键处理 ------------------------------ */

static void on_short_press(void)
{
  if (s_page == PAGE_SET)
  {
    s_page = PAGE_MAIN;
    printf("[UI] back to main\r\n");
  }
  else
  {
    s_mode = (s_mode == MODE_AUTO) ? MODE_MANUAL : MODE_AUTO;
    printf("[MODE] %s\r\n", (s_mode == MODE_AUTO) ? "AUTO" : "MANUAL");
    if (s_mode == MODE_AUTO)
    {
      apply_auto_control();
    }
  }
  s_dirty = 1;
}

static void on_long_press(void)
{
  if (s_page == PAGE_MAIN)
  {
    s_page = PAGE_SET;
    s_dirty = 1;
    printf("[UI] threshold setting\r\n");
  }
}

static void button_poll(void)
{
  uint32_t now = HAL_GetTick();
  uint8_t raw = encoder_pressed();

  if (raw != s_btn_raw)
  {
    s_btn_raw = raw;
    s_btn_raw_tick = now;
  }

  if (((now - s_btn_raw_tick) >= BTN_DEBOUNCE_MS) && (raw != s_btn_level))
  {
    s_btn_level = raw;
    if (s_btn_level != 0u)
    {
      s_btn_press_tick = now;
      s_btn_long_fired = 0;
    }
    else if ((s_btn_long_fired == 0u) && ((now - s_btn_press_tick) < BTN_LONG_MS))
    {
      on_short_press();
    }
    else
    {
      /* 长按已在按下期间处理 */
    }
  }

  if ((s_btn_level != 0u) && (s_btn_long_fired == 0u) &&
      ((now - s_btn_press_tick) >= BTN_LONG_MS))
  {
    s_btn_long_fired = 1;
    on_long_press();
  }
}

/* ------------------------------ 采集与控制 ------------------------------ */

static void sensor_poll(void)
{
  uint32_t now = HAL_GetTick();
  uint32_t sum = 0;
  uint16_t raw;
  uint8_t light;
  uint8_t i;

  if ((now - s_last_sample) < SAMPLE_PERIOD_MS)
  {
    return;
  }
  s_last_sample = now;

  for (i = 0; i < 8u; i++)
  {
    sum += adc_read_raw();
  }
  raw = (uint16_t)(sum / 8u);

  light = (uint8_t)(((uint32_t)raw * 100u) / 4095u);
#if (LIGHT_DARK_WHEN_LOW == 0)
  light = (uint8_t)(100u - light);
#endif

  if ((s_light_valid == 0u) || (light != s_light))
  {
    s_light = light;
    s_light_valid = 1;
    s_dirty = 1;
  }

  apply_auto_control();
}

/* ------------------------------ 界面显示 ------------------------------ */

static void render_main(void)
{
  char buf[12];
  uint8_t hint_alt = (uint8_t)((HAL_GetTick() / HINT_PERIOD_MS) & 1u);
  const char *hint;

  oled_draw_text(0, 0, "光照");
  (void)snprintf(buf, sizeof(buf), "%u%%", (unsigned int)s_light);
  oled_draw_text(40, 0, buf);

  oled_draw_text(0, 16, "阈值");
  (void)snprintf(buf, sizeof(buf), "%u%%", (unsigned int)s_threshold);
  oled_draw_text(40, 16, buf);

  oled_draw_text(0, 32, "模式");
  oled_draw_text(40, 32, (s_mode == MODE_AUTO) ? "自动" : "手动");

  oled_draw_text(0, 48, "灯");
  oled_draw_text(24, 48, (s_lamp != 0u) ? "开" : "关");

  if (hint_alt != 0u)
  {
    hint = (s_mode == MODE_MANUAL) ? "旋转开关灯" : "旋转调阈值";
  }
  else
  {
    hint = "长按设置";
  }
  oled_draw_text((uint8_t)(128u - oled_text_width(hint)), 48, hint);
}

static void render_setting(void)
{
  char buf[12];

  oled_draw_text_center(0, "阈值设置");
  (void)snprintf(buf, sizeof(buf), "[%u%%]", (unsigned int)s_threshold);
  oled_draw_text_center(16, buf);
  oled_draw_text_center(32, "旋转调整");
  oled_draw_text_center(48, "短按返回");
}

static void render(void)
{
  oled_clear();
  if (s_page == PAGE_MAIN)
  {
    render_main();
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
    if ((now - s_oled_retry) >= OLED_RETRY_MS)
    {
      s_oled_retry = now;
      if (oled_init() != 0u)
      {
        s_dirty = 1;
      }
    }
    return;
  }

  if ((s_dirty == 0u) && ((now - s_last_render) < UI_PERIOD_MS))
  {
    return;
  }

  render();
  s_dirty = 0;
  s_last_render = now;
}

/* ------------------------------ 对外接口 ------------------------------ */

void app_init(void)
{
  (void)setvbuf(stdout, NULL, _IONBF, 0);

  led_init();
  relay_write(0);
  s_lamp = 0;

  encoder_init();
  adc_init();

  s_page = PAGE_MAIN;
  s_mode = MODE_AUTO;
  s_threshold = APP_THRESHOLD_DEFAULT;
  s_light = 0;
  s_light_valid = 0;
  s_dirty = 1;

  printf("=== Project 02: STM32 Light Control ===\r\n");
  printf("[SYS] HCLK=%lu Hz\r\n", (unsigned long)HAL_RCC_GetHCLKFreq());
  printf("[CFG] threshold=%u%% step=%u%% hysteresis=%u%%\r\n",
         (unsigned int)s_threshold, (unsigned int)APP_THRESHOLD_STEP,
         (unsigned int)APP_HYSTERESIS);

  if (oled_init() != 0u)
  {
    printf("[OLED] ok, %s addr=0x%02X\r\n", oled_bus_name(), (unsigned int)oled_i2c_addr());
  }
  else
  {
    printf("[OLED] not found, check VCC/GND/SCL/SDA\r\n");
  }
}

void app_task(void)
{
  int8_t dir;

  /* 心跳灯 */
  led_set((HAL_GetTick() % 1000u) < 500u ? 1u : 0u);

  button_poll();

  dir = encoder_poll();
  if (dir != 0)
  {
    if ((s_page == PAGE_SET) || (s_mode == MODE_AUTO))
    {
      adjust_threshold(dir);
    }
    else
    {
      set_lamp((dir > 0) ? 1u : 0u);   /* 手动模式：顺时针开灯、逆时针关灯 */
    }
  }

  sensor_poll();
  ui_task();

  HAL_Delay(2);
}
