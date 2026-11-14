/**
  ******************************************************************************
  * @file    app.c
  * @brief   智能声音检测与报警：ADC 采声音强度 + 编码器调阈值 + 蜂鸣器报警
  *
  * 交互：
  *   编码器旋钮：调报警阈值（5%~95%，每格 5%）
  *   没有按键：上电即工作，声音超过阈值就报警并计数
  ******************************************************************************
  */
#include "app.h"
#include "adc.h"
#include "board.h"
#include "encoder.h"
#include "i2c.h"
#include "oled.h"
#include <stdio.h>

#define SAMPLE_PERIOD_MS   20u     /* 50Hz 采样，兼顾响应和显示稳定 */
#define UI_PERIOD_MS       150u
#define OLED_RETRY_MS      1000u
#define BLINK_PERIOD_MS    400u
#define BEEP_ON_MS         120u
#define BEEP_PERIOD_MS     1000u
#define ALARM_COUNT_MAX    9999u

static uint8_t  s_threshold;
static uint8_t  s_level;          /* 平滑后的声音强度 0~100 */
static uint8_t  s_alarm;
static uint16_t s_alarm_count;

static uint32_t s_last_sample;
static uint32_t s_last_render;
static uint32_t s_oled_retry;

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

static void buzzer_set(uint8_t on)
{
  HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN,
                    (on != 0u) ? BUZZER_ON_LEVEL : BUZZER_OFF_LEVEL);
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
  printf("[TH] %u%%\r\n", (unsigned int)s_threshold);
}

/* ------------------------------ 采集与报警 ------------------------------ */

static void sensor_poll(void)
{
  uint32_t now = HAL_GetTick();
  uint32_t sum = 0;
  uint16_t raw;
  uint8_t level;
  uint8_t i;

  if ((now - s_last_sample) < SAMPLE_PERIOD_MS)
  {
    return;
  }
  s_last_sample = now;

  for (i = 0; i < 4u; i++)
  {
    sum += adc_read_channel(SOUND_ADC_CHANNEL);
  }
  raw = (uint16_t)(sum / 4u);
#if (SOUND_OUT_MODE == SOUND_OUT_ANALOG)
  level = (uint8_t)(((uint32_t)raw * 100u) / 4095u);
#else
  level = (raw > 2048u) ? 100u : 0u;   /* 数字输出：高于中点算“有声音” */
#endif

  /* 快起慢落：新值权重 3/4，声音峰值不会被平滑掉，回落时更平滑 */
  s_level = (uint8_t)((s_level + (uint16_t)level * 3u) / 4u);

  /* 报警判断：超过阈值触发，回落到“阈值 - 回差”以下才解除 */
  if ((s_alarm == 0u) && (s_level >= s_threshold))
  {
    s_alarm = 1;
    if (s_alarm_count < ALARM_COUNT_MAX)
    {
      s_alarm_count++;
    }
    printf("[ALARM] count=%u level=%u%%\r\n",
           (unsigned int)s_alarm_count, (unsigned int)s_level);
  }
  else if ((s_alarm != 0u) && (s_level <= (uint8_t)(s_threshold - APP_ALARM_HYST)))
  {
    s_alarm = 0;
    printf("[ALARM] clear\r\n");
  }

}

/* ------------------------------ 界面显示 ------------------------------ */

static void render(void)
{
  char buf[16];
  uint32_t now = HAL_GetTick();
  uint8_t blink_on = (uint8_t)(((now / BLINK_PERIOD_MS) & 1u) != 0u);

  oled_clear();

  oled_draw_text(0, 0, "声音");
  (void)snprintf(buf, sizeof(buf), "%u%%", (unsigned int)s_level);
  oled_draw_text(40, 0, buf);

  oled_draw_text(0, 16, "阈值");
  (void)snprintf(buf, sizeof(buf), "%u%%", (unsigned int)s_threshold);
  oled_draw_text(40, 16, buf);

  oled_draw_text(0, 32, "状态");
  if (s_alarm != 0u)
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
  (void)snprintf(buf, sizeof(buf), "%u次", (unsigned int)s_alarm_count);
  oled_draw_text(40, 48, buf);

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
      (void)oled_init();
    }
    return;
  }

  if ((now - s_last_render) < UI_PERIOD_MS)
  {
    return;
  }

  render();
  s_last_render = now;
}

/* ------------------------------ 对外接口 ------------------------------ */

void app_init(void)
{
  (void)setvbuf(stdout, NULL, _IONBF, 0);

  led_init();
  buzzer_set(0);

  encoder_init();
  adc_init();

  s_threshold = APP_THRESHOLD_DEFAULT;
  s_level = 0;
  s_alarm = 0;
  s_alarm_count = 0;

  printf("=== Project 03: STM32 Sound Detection & Alarm ===\r\n");
  printf("[SYS] HCLK=%lu Hz\r\n", (unsigned long)HAL_RCC_GetHCLKFreq());
  printf("[CFG] threshold=%u%% step=%u%% hysteresis=%u%%\r\n",
         (unsigned int)s_threshold, (unsigned int)APP_THRESHOLD_STEP,
         (unsigned int)APP_ALARM_HYST);

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

  dir = encoder_poll();
  if (dir != 0)
  {
    threshold_step(dir);
  }

  sensor_poll();

  /* 报警时蜂鸣器间歇鸣叫 */
  if (s_alarm != 0u)
  {
    buzzer_set(((HAL_GetTick() % BEEP_PERIOD_MS) < BEEP_ON_MS) ? 1u : 0u);
  }
  else
  {
    buzzer_set(0);
  }

  ui_task();

  HAL_Delay(2);
}
