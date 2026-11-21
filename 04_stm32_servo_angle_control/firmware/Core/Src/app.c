/**
  ******************************************************************************
  * @file    app.c
  * @brief   自动闸门：三种模式（手动 / 往复 / 自动）
  *
  * 交互：
  *   编码器转一格：手动 → 往复 → 自动 循环
  *
  *   手动模式：目标值固定 90°，上电/进入时当前值回到 0°
  *             按一次触摸 → 当前值转到 90°；再按一次 → 回到 0°
  *   往复模式：当前值在 0° 和 180° 之间自动来回
  *   自动模式：目标值固定 90°，上电/进入时当前值回到 0°
  *             红外检测到有东西 → 当前值转到 90°
  *             红外再次变化（东西离开）→ 当前值回到 0°
  *
  *   长按触摸：任何模式下强制关闸（当前值回 0°）
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
  MODE_SWEEP,
  MODE_AUTO,
  MODE_COUNT
} mode_t;

static mode_t   s_mode;
static uint8_t  s_cmd;          /* 指令角度（舵机要去的方向） */
static uint8_t  s_current;      /* 当前角度（缓动后的实际输出） */
static uint8_t  s_arrived;

static uint8_t  s_ir_raw;
static uint8_t  s_ir_present;
static uint32_t s_ir_change_tick;

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

static void set_cmd(uint8_t angle)
{
  if (angle > SERVO_ANGLE_MAX)
  {
    angle = SERVO_ANGLE_MAX;
  }
  if (angle == s_cmd)
  {
    return;
  }
  s_cmd = angle;
  s_arrived = 0;
  printf("[GATE] -> %u deg\r\n", (unsigned int)angle);
}

/* 屏幕上的「目标」：手动/自动固定 90°，往复跟随摆动端点 */
static uint8_t display_target(void)
{
  if (s_mode == MODE_SWEEP)
  {
    return s_cmd;
  }
  return APP_OPEN_ANGLE;
}

static const char *mode_name(void)
{
  if (s_mode == MODE_MANUAL)
  {
    return "手动";
  }
  if (s_mode == MODE_SWEEP)
  {
    return "往复";
  }
  return "自动";
}

/* ------------------------------ 红外检测 ------------------------------ */

static uint8_t ir_active(void)
{
  return (HAL_GPIO_ReadPin(IR_PORT, IR_PIN) == IR_ACTIVE_LEVEL) ? 1u : 0u;
}

static void on_ir_changed(void)
{
  printf("[IR] %s\r\n", (s_ir_present != 0u) ? "object" : "clear");

  if (s_mode != MODE_AUTO)
  {
    return;                            /* 只有自动模式响应红外 */
  }

  if (s_ir_present != 0u)
  {
    set_cmd(APP_OPEN_ANGLE);           /* 有东西经过 → 开闸 */
  }
  else
  {
    set_cmd(APP_CLOSE_ANGLE);          /* 东西离开 → 关闸 */
  }
  beep(APP_BEEP_ARRIVE_MS);
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

/* ------------------------------ 模式切换 ------------------------------ */

static void enter_mode(mode_t mode)
{
  s_mode = mode;

  if (mode == MODE_SWEEP)
  {
    set_cmd((s_current < 90u) ? APP_SWEEP_FAR : APP_CLOSE_ANGLE);
  }
  else
  {
    set_cmd(APP_CLOSE_ANGLE);          /* 手动/自动：进入时当前值回 0° */
  }

  beep(APP_BEEP_ARRIVE_MS);
  printf("[MODE] %s\r\n", mode_name());
}

static void encoder_apply(int8_t dir)
{
  (void)dir;
  enter_mode((mode_t)((s_mode + 1) % MODE_COUNT));
}

/* ------------------------------ 触摸处理 ------------------------------ */

static void on_short_press(void)
{
  if (s_mode != MODE_MANUAL)
  {
    return;                            /* 只有手动模式用触摸开/关闸 */
  }

  if (s_cmd == APP_CLOSE_ANGLE)
  {
    set_cmd(APP_OPEN_ANGLE);
    printf("[MANUAL] open\r\n");
  }
  else
  {
    set_cmd(APP_CLOSE_ANGLE);
    printf("[MANUAL] close\r\n");
  }
  beep(APP_BEEP_ARRIVE_MS);
}

static void on_long_press(void)
{
  set_cmd(APP_CLOSE_ANGLE);
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

/* ------------------------------ 角度控制 ------------------------------ */

static void ramp_task(void)
{
  uint32_t now = HAL_GetTick();

  if ((now - s_last_ramp) < APP_RAMP_MS)
  {
    return;
  }
  s_last_ramp = now;

  if (s_current < s_cmd)
  {
    s_current = (uint8_t)((s_cmd - s_current > APP_RAMP_STEP) ?
                          (s_current + APP_RAMP_STEP) : s_cmd);
  }
  else if (s_current > s_cmd)
  {
    s_current = (uint8_t)((s_current - s_cmd > APP_RAMP_STEP) ?
                          (s_current - APP_RAMP_STEP) : s_cmd);
  }
  else
  {
    /* 已经到位 */
  }

  servo_set_angle(s_current);

  if ((s_current == s_cmd) && (s_arrived == 0u))
  {
    s_arrived = 1;
    beep(APP_BEEP_ARRIVE_MS);

    if (s_mode == MODE_SWEEP)
    {
      /* 往复模式：到头后自动折返 */
      s_cmd = (s_cmd >= APP_SWEEP_FAR) ? APP_CLOSE_ANGLE : APP_SWEEP_FAR;
      s_arrived = 0;
    }
  }
}

/* ------------------------------ 界面显示 ------------------------------ */

static void render(void)
{
  char buf[16];
  const char *status;

  if (s_current != s_cmd)
  {
    status = "转动中";
  }
  else if ((s_mode == MODE_AUTO) && (s_ir_present != 0u))
  {
    status = "有车";
  }
  else
  {
    status = "到位";
  }

  oled_clear();

  oled_draw_text(0, 0, "模式");
  oled_draw_text(40, 0, mode_name());

  oled_draw_text(0, 16, "目标");
  (void)snprintf(buf, sizeof(buf), "%u°", (unsigned int)display_target());
  oled_draw_text(40, 16, buf);

  oled_draw_text(0, 32, "当前");
  (void)snprintf(buf, sizeof(buf), "%u°", (unsigned int)s_current);
  oled_draw_text(40, 32, buf);

  oled_draw_text(0, 48, "状态");
  oled_draw_text(40, 48, status);

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

  s_mode = MODE_MANUAL;
  s_current = APP_CLOSE_ANGLE;
  s_cmd = APP_CLOSE_ANGLE;
  s_arrived = 1;
  s_ir_raw = ir_active();
  s_ir_present = 0;
  servo_set_angle(APP_CLOSE_ANGLE);

  printf("=== Project 04: STM32 Auto Gate (3 modes) ===\r\n");
  printf("[SYS] HCLK=%lu Hz\r\n", (unsigned long)HAL_RCC_GetHCLKFreq());
  printf("[TOUCH] idle high=%lu/50 -> active level %s\r\n",
         (unsigned long)high_cnt, (s_touch_active_level == GPIO_PIN_SET) ? "HIGH" : "LOW");
  printf("[CFG] open=%u close=%u sweep=%u debounce=%ums\r\n",
         (unsigned int)APP_OPEN_ANGLE, (unsigned int)APP_CLOSE_ANGLE,
         (unsigned int)APP_SWEEP_FAR, (unsigned int)APP_IR_DEBOUNCE_MS);

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

  ramp_task();
  buzzer_task();
  ui_task();

  HAL_Delay(2);
}
