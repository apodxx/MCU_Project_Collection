/**
  ******************************************************************************
  * @file    app.c
  * @brief   自动闸门：红外检测车辆 + 自动/手动两种模式 + 舵机抬杆
  *
  * 交互：
  *   编码器转一格：自动 ↔ 手动 切换
  *   自动模式：红外检测到车 → 开闸；车走后 3 秒 → 自动关闸
  *   手动模式：红外检测到车 → 蜂鸣提醒；按触摸 → 开闸/关闸
  *   长按触摸：立即关闸（两种模式都生效）
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
#define BLINK_PERIOD_MS    400u

typedef enum
{
  MODE_AUTO = 0,
  MODE_MANUAL,
  MODE_COUNT
} mode_t;

static mode_t   s_mode;
static uint8_t  s_target;
static uint8_t  s_current;
static uint8_t  s_arrived;

static uint8_t  s_ir_raw;
static uint8_t  s_ir_present;
static uint32_t s_ir_change_tick;
static uint32_t s_auto_close_tick;      /* 0 = 没在计时 */

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
  buzzer_set((HAL_GetTick() < s_beep_until) ? 1u : 0u);
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
  printf("[GATE] target %u deg\r\n", (unsigned int)angle);
}

/* ------------------------------ 红外检测 ------------------------------ */

static uint8_t ir_active(void)
{
  return (HAL_GPIO_ReadPin(IR_PORT, IR_PIN) == IR_ACTIVE_LEVEL) ? 1u : 0u;
}

static void on_ir_changed(void)
{
  if (s_ir_present != 0u)
  {
    printf("[IR] car detected\r\n");
    if (s_mode == MODE_MANUAL)
    {
      beep(APP_BEEP_CAR_MS);          /* 手动模式：提醒有车，等按键 */
    }
    else
    {
      set_target(APP_GATE_OPEN_ANGLE); /* 自动模式：直接开闸 */
      s_auto_close_tick = 0;
    }
  }
  else
  {
    printf("[IR] clear\r\n");
    if ((s_mode == MODE_AUTO) && (s_target == APP_GATE_OPEN_ANGLE))
    {
      s_auto_close_tick = HAL_GetTick() + APP_AUTO_CLOSE_MS;
    }
  }
}

static void ir_task(void)
{
  uint32_t now = HAL_GetTick();
  uint8_t raw = ir_active();

  if (raw != s_ir_raw)
  {
    s_ir_raw = raw;
    s_ir_change_tick = now;
  }

  if (((now - s_ir_change_tick) >= APP_IR_DEBOUNCE_MS) && (raw != s_ir_present))
  {
    s_ir_present = raw;
    on_ir_changed();
  }
}

/* 自动模式：车在就保持开闸；车走后延时关闸 */
static void auto_task(void)
{
  uint32_t now = HAL_GetTick();

  if (s_mode != MODE_AUTO)
  {
    s_auto_close_tick = 0;
    return;
  }

  if (s_ir_present != 0u)
  {
    if (s_target != APP_GATE_OPEN_ANGLE)
    {
      set_target(APP_GATE_OPEN_ANGLE);
    }
    s_auto_close_tick = 0;
    return;
  }

  if ((s_auto_close_tick != 0u) && (now >= s_auto_close_tick))
  {
    s_auto_close_tick = 0;
    if (s_target == APP_GATE_OPEN_ANGLE)
    {
      set_target(APP_GATE_CLOSE_ANGLE);
      beep(APP_BEEP_ARRIVE_MS);
    }
  }
}

/* ------------------------------ 模式切换 ------------------------------ */

static void on_short_press(void)
{
  if (s_mode != MODE_MANUAL)
  {
    return;                            /* 自动模式下触摸不动作 */
  }

  if (s_target == APP_GATE_CLOSE_ANGLE)
  {
    set_target(APP_GATE_OPEN_ANGLE);
    printf("[MANUAL] open\r\n");
  }
  else
  {
    set_target(APP_GATE_CLOSE_ANGLE);
    printf("[MANUAL] close\r\n");
  }
  beep(APP_BEEP_ARRIVE_MS);
}

static void on_long_press(void)
{
  set_target(APP_GATE_CLOSE_ANGLE);
  beep(APP_BEEP_LIMIT_MS);
  printf("[GATE] force close\r\n");
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

/* 编码器只负责切模式：自动 ↔ 手动 */
static void encoder_apply(int8_t dir)
{
  (void)dir;
  s_mode = (s_mode == MODE_AUTO) ? MODE_MANUAL : MODE_AUTO;
  s_auto_close_tick = 0;

  if (s_mode == MODE_AUTO)
  {
    if (s_ir_present != 0u)
    {
      set_target(APP_GATE_OPEN_ANGLE);
    }
  }
  else if (s_ir_present != 0u)
  {
    beep(APP_BEEP_CAR_MS);            /* 切到手动时已经有车：提醒一下 */
  }

  printf("[MODE] %s\r\n", (s_mode == MODE_AUTO) ? "AUTO" : "MANUAL");
  beep(APP_BEEP_ARRIVE_MS);
}

/* ------------------------------ 角度控制 ------------------------------ */

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
  }
}

/* ------------------------------ 界面显示 ------------------------------ */

static void render(void)
{
  char buf[16];
  const char *status;
  uint8_t waiting = 0;
  uint32_t now = HAL_GetTick();
  uint8_t blink_on = (uint8_t)(((now / BLINK_PERIOD_MS) & 1u) != 0u);

  if (s_current != s_target)
  {
    status = "运动";
  }
  else if (s_target == APP_GATE_OPEN_ANGLE)
  {
    status = "开闸";
  }
  else if ((s_mode == MODE_MANUAL) && (s_ir_present != 0u))
  {
    status = "等待";                    /* 有车，等按触摸 */
    waiting = 1;
  }
  else
  {
    status = "关闸";
  }

  oled_clear();

  oled_draw_text(0, 0, "模式");
  oled_draw_text(40, 0, (s_mode == MODE_AUTO) ? "自动" : "手动");

  oled_draw_text(0, 16, "红外");
  oled_draw_text(40, 16, (s_ir_present != 0u) ? "有车" : "无车");

  oled_draw_text(0, 32, "角度");
  (void)snprintf(buf, sizeof(buf), "%u°", (unsigned int)s_current);
  oled_draw_text(40, 32, buf);

  oled_draw_text(0, 48, "状态");
  if ((waiting != 0u) && (blink_on == 0u))
  {
    /* 等待时闪烁，提醒按触摸 */
  }
  else
  {
    oled_draw_text(40, 48, status);
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

  /* 红外输入：上拉，检测到遮挡时模块拉低 */
  gpio.Pin = IR_PIN;
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(IR_PORT, &gpio);

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

  s_mode = MODE_AUTO;
  s_current = APP_GATE_CLOSE_ANGLE;
  s_target = APP_GATE_CLOSE_ANGLE;
  s_arrived = 1;
  s_ir_raw = ir_active();
  s_ir_present = 0;
  servo_set_angle(APP_GATE_CLOSE_ANGLE);

  printf("=== Project 04: STM32 Auto Gate (IR + servo) ===\r\n");
  printf("[SYS] HCLK=%lu Hz\r\n", (unsigned long)HAL_RCC_GetHCLKFreq());
  printf("[TOUCH] idle high=%lu/50 -> active level %s\r\n",
         (unsigned long)high_cnt, (s_touch_active_level == GPIO_PIN_SET) ? "HIGH" : "LOW");
  printf("[CFG] mode=AUTO open=%u close=%u auto-close=%ums\r\n",
         (unsigned int)APP_GATE_OPEN_ANGLE, (unsigned int)APP_GATE_CLOSE_ANGLE,
         (unsigned int)APP_AUTO_CLOSE_MS);

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

  ir_task();
  button_poll();

  dir = encoder_poll();
  if (dir != 0)
  {
    encoder_apply(dir);
  }

  auto_task();
  ramp_task();
  buzzer_task();
  ui_task();

  HAL_Delay(2);
}
