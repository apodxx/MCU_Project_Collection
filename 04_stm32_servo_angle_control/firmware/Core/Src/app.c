/**
  ******************************************************************************
  * @file    app.c
  * @brief   舵机角度控制：编码器拧角度 + 触摸切模式 + 蜂鸣器提示
  *
  * 交互：
  *   旋转编码器：手动模式调角度（每格 5°）；预设模式切换预设点
  *   触摸短按  ：手动 → 预设 → 往复 循环
  *   触摸长按  ：回到中位 90°，并切回手动
  ******************************************************************************
  */
#include "app.h"
#include "board.h"
#include "encoder.h"
#include "i2c.h"
#include "oled.h"
#include "pwm.h"
#include <stdio.h>

#define UI_PERIOD_MS       150u
#define OLED_RETRY_MS      1000u
#define BTN_DEBOUNCE_MS    20u
#define BTN_LONG_MS        800u

typedef enum
{
  MODE_MANUAL = 0,
  MODE_PRESET,
  MODE_SWEEP,
  MODE_COUNT
} mode_t;

static const uint8_t s_presets[5] = {0u, 45u, 90u, 135u, 180u};

static mode_t   s_mode;
static uint8_t  s_target;
static uint8_t  s_current;
static uint8_t  s_preset;
static uint8_t  s_arrived;
static uint32_t s_last_ramp;
static uint32_t s_beep_until;

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

static void buzzer_set(uint8_t on)
{
  HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN,
                    (on != 0u) ? BUZZER_ON_LEVEL : BUZZER_OFF_LEVEL);
}

static void beep(uint32_t ms)
{
  s_beep_until = HAL_GetTick() + ms;
}

static void buzzer_task(void)
{
  if (HAL_GetTick() < s_beep_until)
  {
    buzzer_set(1);
  }
  else
  {
    buzzer_set(0);
  }
}

static void set_target(uint8_t angle)
{
  if (angle > SERVO_ANGLE_MAX)
  {
    angle = SERVO_ANGLE_MAX;
  }
  if (angle == s_target)
  {
    return;
  }
  s_target = angle;
  s_arrived = 0;
  printf("[TARGET] %u\r\n", (unsigned int)angle);
}

/* ------------------------------ 模式切换 ------------------------------ */

static const char *mode_name(char *buf, size_t size)
{
  if (s_mode == MODE_MANUAL)
  {
    return "手动";
  }
  if (s_mode == MODE_PRESET)
  {
    (void)snprintf(buf, size, "预设%u", (unsigned int)(s_preset + 1u));
    return buf;
  }
  return "往复";
}

static void on_short_press(void)
{
  if (s_mode == MODE_MANUAL)
  {
    /* 手动模式：每按一次，栏杆抬 15°，到顶后回到 0° */
    uint16_t next = (uint16_t)s_target + APP_MANUAL_STEP;

    if (next > SERVO_ANGLE_MAX)
    {
      next = 0;
    }
    set_target((uint8_t)next);
    beep(APP_BEEP_ARRIVE_MS);
    printf("[MANUAL] -> %u deg\r\n", (unsigned int)next);
  }
  else if (s_mode == MODE_PRESET)
  {
    /* 预设模式：按一次切到下一个预置位 */
    s_preset = (uint8_t)((s_preset + 1u) % 5u);
    set_target(s_presets[s_preset]);
    beep(APP_BEEP_ARRIVE_MS);
    printf("[PRESET] %u (%u deg)\r\n", (unsigned int)(s_preset + 1u),
           (unsigned int)s_presets[s_preset]);
  }
  else
  {
    /* 往复模式自动运行，短按不动作 */
  }
}

static void on_long_press(void)
{
  s_mode = MODE_MANUAL;
  set_target(APP_ANGLE_HOME);
  beep(APP_BEEP_LIMIT_MS);
  printf("[MODE] HOME %u (gate closed)\r\n", (unsigned int)APP_ANGLE_HOME);
}

static void button_poll(void)
{
  uint32_t now = HAL_GetTick();
  uint8_t raw = (HAL_GPIO_ReadPin(TOUCH_PORT, TOUCH_PIN) == s_touch_active_level) ? 1u : 0u;

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

/* ------------------------------ 角度控制 ------------------------------ */

static void enter_mode(mode_t mode)
{
  s_mode = mode;

  if (mode == MODE_MANUAL)
  {
    set_target(s_current);                        /* 手动：从当前角度接着按 */
  }
  else if (mode == MODE_PRESET)
  {
    set_target(s_presets[s_preset]);              /* 预设：先到当前预置位 */
  }
  else
  {
    set_target((s_current < 90u) ? 180u : 0u);    /* 往复：先摆向另一头 */
  }

  beep(APP_BEEP_ARRIVE_MS);
  printf("[MODE] %s\r\n", (mode == MODE_MANUAL) ? "MANUAL" :
                          (mode == MODE_PRESET) ? "PRESET" : "SWEEP");
}

/* 编码器只负责切模式：转一格切一个，循环 */
static void encoder_apply(int8_t dir)
{
  int16_t mode = (int16_t)s_mode + (int16_t)dir;

  if (mode < 0)
  {
    mode = (int16_t)MODE_COUNT - 1;
  }
  if (mode >= (int16_t)MODE_COUNT)
  {
    mode = 0;
  }
  enter_mode((mode_t)mode);
}

/* 每 20ms 让当前角度向目标走一步，形成缓动 */
static void ramp_task(void)
{
  uint32_t now = HAL_GetTick();

  if ((now - s_last_ramp) < APP_RAMP_MS)
  {
    return;
  }
  s_last_ramp = now;

  if (s_current < s_target)
  {
    s_current = (uint8_t)((s_target - s_current > APP_RAMP_STEP) ?
                          (s_current + APP_RAMP_STEP) : s_target);
  }
  else if (s_current > s_target)
  {
    s_current = (uint8_t)((s_current - s_target > APP_RAMP_STEP) ?
                          (s_current - APP_RAMP_STEP) : s_target);
  }
  else
  {
    /* 已经到位 */
  }

  servo_set_angle(s_current);

  if ((s_current == s_target) && (s_arrived == 0u))
  {
    s_arrived = 1;
    beep(APP_BEEP_ARRIVE_MS);
    if (s_mode == MODE_SWEEP)
    {
      set_target((s_target >= 180u) ? 0u : 180u);
    }
  }
}

/* ------------------------------ 界面显示 ------------------------------ */

static void render(void)
{
  char buf[16];
  char mode_buf[16];

  oled_clear();

  oled_draw_text(0, 0, "模式");
  oled_draw_text(40, 0, mode_name(mode_buf, sizeof(mode_buf)));

  oled_draw_text(0, 16, "目标");
  (void)snprintf(buf, sizeof(buf), "%u°", (unsigned int)s_target);
  oled_draw_text(40, 16, buf);

  oled_draw_text(0, 32, "当前");
  (void)snprintf(buf, sizeof(buf), "%u°", (unsigned int)s_current);
  oled_draw_text(40, 32, buf);

  oled_draw_text(0, 48, "状态");
  oled_draw_text(40, 48, (s_arrived != 0u) ? "到位" : "转动中");

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
  GPIO_InitTypeDef gpio = {0};
  uint32_t high_cnt = 0;
  uint8_t i;

  (void)setvbuf(stdout, NULL, _IONBF, 0);

  led_init();
  buzzer_set(0);

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
  pwm_init();

  s_mode = MODE_MANUAL;
  s_current = APP_ANGLE_HOME;
  s_target = APP_ANGLE_HOME;
  s_arrived = 1;
  s_preset = 2u;
  servo_set_angle(APP_ANGLE_HOME);

  printf("=== Project 04: STM32 Servo Angle Control ===\r\n");
  printf("[SYS] HCLK=%lu Hz\r\n", (unsigned long)HAL_RCC_GetHCLKFreq());
  printf("[TOUCH] idle high=%lu/50 -> active level %s\r\n",
         (unsigned long)high_cnt, (s_touch_active_level == GPIO_PIN_SET) ? "HIGH" : "LOW");
  printf("[CFG] start=%u deg step=%u deg ramp=%u deg/%ums\r\n",
         (unsigned int)APP_ANGLE_HOME, (unsigned int)APP_MANUAL_STEP,
         (unsigned int)APP_RAMP_STEP, (unsigned int)APP_RAMP_MS);

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
    encoder_apply(dir);
  }

  ramp_task();
  buzzer_task();
  ui_task();

  HAL_Delay(2);
}
