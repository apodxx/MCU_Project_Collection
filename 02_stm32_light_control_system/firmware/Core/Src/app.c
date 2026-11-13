/**
  ******************************************************************************
  * @file    app.c
  * @brief   智能光照控制：ADC 采光 + 旋转编码器调阈值 + 触摸键切模式
  *
  * 交互：
  *   编码器旋钮：调光照阈值（5%~95%，每格 5%）
  *   触摸短按  ：自动 / 手动 切换
  *   触摸长按  ：手动模式下开灯 / 关灯（自动模式下无动作）
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

typedef enum
{
  MODE_AUTO = 0,
  MODE_MANUAL
} mode_t;

static mode_t   s_mode;
static uint8_t  s_threshold;
static uint8_t  s_light;
static uint8_t  s_light_valid;
static uint8_t  s_lamp;

static uint32_t s_last_sample;
static uint8_t  s_dirty;
static uint32_t s_last_render;
static uint32_t s_oled_retry;

static uint8_t  s_touch_level;
static uint8_t  s_touch_raw;
static uint32_t s_touch_raw_tick;
static uint32_t s_touch_press_tick;
static uint8_t  s_touch_long_fired;
static uint8_t  s_touch_active_level = TOUCH_ACTIVE_LEVEL;   /* 上电自动识别 */

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

static void actuator_write(uint8_t on)
{
  HAL_GPIO_WritePin(ACTUATOR_PORT, ACTUATOR_PIN, (on != 0u) ? ACTUATOR_ON_LEVEL : ACTUATOR_OFF_LEVEL);
}

static void set_lamp(uint8_t on)
{
  if (on == s_lamp)
  {
    return;
  }
  s_lamp = on;
  actuator_write(on);
  s_dirty = 1;
  printf("[LAMP] %s\r\n", (on != 0u) ? "ON" : "OFF");
}

static uint8_t touch_pressed(void)
{
  return (HAL_GPIO_ReadPin(TOUCH_PORT, TOUCH_PIN) == s_touch_active_level) ? 1u : 0u;
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
    /* 回差区间内保持当前状态，避免输出频繁开关 */
  }
}

/* ------------------------------ 参数调整 ------------------------------ */

static void threshold_step(int8_t dir)
{
  int16_t value = (int16_t)s_threshold + ((int16_t)dir * (int16_t)APP_THRESHOLD_STEP);

  if (value > (int16_t)APP_THRESHOLD_MAX)
  {
    value = (int16_t)APP_THRESHOLD_MIN;
  }
  if (value < (int16_t)APP_THRESHOLD_MIN)
  {
    value = (int16_t)APP_THRESHOLD_MAX;
  }

  s_threshold = (uint8_t)value;
  s_dirty = 1;
  printf("[TH] %u%%\r\n", (unsigned int)s_threshold);
  apply_auto_control();
}

/* ------------------------------ 按键处理 ------------------------------ */

static void on_short_press(void)
{
  s_mode = (s_mode == MODE_AUTO) ? MODE_MANUAL : MODE_AUTO;
  s_dirty = 1;
  printf("[MODE] %s\r\n", (s_mode == MODE_AUTO) ? "AUTO" : "MANUAL");
  if (s_mode == MODE_AUTO)
  {
    apply_auto_control();
  }
}

static void on_long_press(void)
{
  if (s_mode == MODE_MANUAL)
  {
    set_lamp((s_lamp == 0u) ? 1u : 0u);
    printf("[LAMP] manual %s\r\n", (s_lamp != 0u) ? "ON" : "OFF");
  }
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

  if (((now - s_touch_raw_tick) >= BTN_DEBOUNCE_MS) && (raw != s_touch_level))
  {
    s_touch_level = raw;
    if (s_touch_level != 0u)
    {
      s_touch_press_tick = now;
      s_touch_long_fired = 0;
    }
    else if ((s_touch_long_fired == 0u) && ((now - s_touch_press_tick) < BTN_LONG_MS))
    {
      on_short_press();
    }
    else
    {
      /* 长按已在按下期间处理 */
    }
  }

  if ((s_touch_level != 0u) && (s_touch_long_fired == 0u) &&
      ((now - s_touch_press_tick) >= BTN_LONG_MS))
  {
    s_touch_long_fired = 1;
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
    sum += adc_read_channel(ADC_CHANNEL_0);
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

static void render(void)
{
  char buf[12];
  uint32_t now = HAL_GetTick();
  uint8_t hint_alt = (uint8_t)((now / HINT_PERIOD_MS) & 1u);
  const char *hint;

  oled_clear();

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
    hint = (s_mode == MODE_AUTO) ? "短按切手动" : "长按开关灯";
  }
  else
  {
    hint = "旋钮调阈值";
  }
  oled_draw_text((uint8_t)(128u - oled_text_width(hint)), 48, hint);

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
  GPIO_InitTypeDef gpio = {0};
  uint32_t high_cnt = 0;
  uint8_t i;

  (void)setvbuf(stdout, NULL, _IONBF, 0);

  led_init();
  actuator_write(0);
  s_lamp = 0;

  /* 触摸输入：下拉，避免悬空误触发 */
  gpio.Pin = TOUCH_PIN;
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_PULLDOWN;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TOUCH_PORT, &gpio);

  /* TTP223 有两种输出极性：上电按空闲电平自动识别（此时别碰触摸板） */
  for (i = 0; i < 50u; i++)
  {
    if (HAL_GPIO_ReadPin(TOUCH_PORT, TOUCH_PIN) == GPIO_PIN_SET)
    {
      high_cnt++;
    }
    HAL_Delay(4);
  }
  s_touch_active_level = (high_cnt >= 40u) ? GPIO_PIN_RESET : GPIO_PIN_SET;

  encoder_init();
  adc_init();

  s_mode = MODE_AUTO;
  s_threshold = APP_THRESHOLD_DEFAULT;
  s_light = 0;
  s_light_valid = 0;
  s_dirty = 1;

  printf("=== Project 02: STM32 Light Control (encoder threshold) ===\r\n");
  printf("[SYS] HCLK=%lu Hz\r\n", (unsigned long)HAL_RCC_GetHCLKFreq());
  printf("[TOUCH] idle high=%lu/50 -> active level %s\r\n",
         (unsigned long)high_cnt, (s_touch_active_level == GPIO_PIN_SET) ? "HIGH" : "LOW");
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
    threshold_step(dir);
  }

  sensor_poll();
  ui_task();

  HAL_Delay(2);
}
