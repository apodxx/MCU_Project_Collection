# 固件工程说明（STM32CubeIDE）

STM32F103C8T6 · 72MHz · HAL 库 · STM32CubeIDE 2.1.1

## 目录结构

```
firmware/
├─ 04_stm32_servo_angle_control.ioc  # CubeMX 配置（引脚、时钟、外设）
├─ Core/
│  ├─ Inc/
│  │  ├─ board.h        # 引脚定义、脉宽范围、角度参数
│  │  ├─ pwm.h          # 舵机 PWM
│  │  ├─ encoder.h      # 旋转编码器正交解码
│  │  ├─ oled.h         # OLED 驱动
│  │  ├─ oled_font.h    # 字库声明
│  │  └─ app.h
│  ├─ Src/
│  │  ├─ main.c         # CubeMX 生成（只在 USER CODE 区加了 3 行）
│  │  ├─ app.c          # 业务逻辑：模式状态机 / 缓动 / 界面
│  │  ├─ pwm.c          # TIM2_CH1 50Hz PWM，手工初始化
│  │  ├─ encoder.c      # A/B 状态表解码
│  │  ├─ oled.c         # SSD1306 显存与绘制（复用项目 01）
│  │  ├─ oled_font_ascii.c / oled_font_cn.c  # 字库（脚本生成）
│  │  ├─ retarget.c     # printf 重定向到 USART1
│  │  └─ gpio.c / i2c.c / usart.c  # CubeMX 生成
│  └─ Startup/
├─ Drivers/             # CMSIS + STM32F1 HAL（含手工加入的 TIM 驱动）
├─ tools/gen_font.py    # 字库生成 / 界面预览
└─ STM32F103C8TX_FLASH.ld
```

## 引脚

| 功能 | 引脚 | 说明 |
| :-- | :-- | :-- |
| 舵机信号 | PA0 | TIM2_CH1，复用推挽，50Hz |
| 编码器 A / B | PA1 / PA2 | 内部上拉输入，正交解码 |
| 触摸 OUT | PA3 | 输入（下拉），极性上电自检 |
| 蜂鸣器 IN | PB5 | 输出，默认低电平触发 |
| OLED SCL / SDA | PB10 / PB11 | I2C2（PB6/PB7 的 I2C1 也可，自动识别） |
| 调试串口 | PA9 / PA10 | USART1，115200 |
| ST-Link | PA13 / PA14 | SWDIO / SWCLK |
| 板载 LED | PC13 | 低电平点亮，心跳指示 |

## 关于 PWM

`.ioc` 里把 **PA0 配成普通 GPIO 输出**（CubeMX 的 TIM 配置在无界面生成时不被识别，
避免工程打开时报冲突），TIM2 在 `pwm.c` 里手工初始化：

```c
PSC = 72 - 1      /* 72MHz / 72 = 1MHz，1 计数 = 1µs */
ARR = 20000 - 1   /* 20ms 周期，50Hz */
CCR = 500 + angle * 2000 / 180   /* 0.5ms ~ 2.5ms */
```

`stm32f1xx_hal_conf.h` 里的 `HAL_TIM_MODULE_ENABLED` 已打开，
`Drivers/STM32F1xx_HAL_Driver` 下补了 `stm32f1xx_hal_tim.c / _tim_ex.c`。

## 编译 / 烧录

```text
File > Open Projects from File System... -> 选本目录 -> Ctrl+B -> ▶ 烧录
```

## 代码结构说明

- **main.c 只改了三处**：`USER CODE Includes` 加 `app.h`，`USER CODE 2` 调 `app_init()`，
  `USER CODE 3` 调 `app_task()`，CubeMX 重新生成代码不会覆盖。
- **pwm.c**：`servo_set_angle(angle)` 把 0~180° 映射成 500~2500µs，直接写 CCR。
- **encoder.c**：16 项状态表解码，`ENC_COUNTS_PER_STEP` 控制每格跳变数。
- **app.c**：三态状态机（手动/预设/往复）+ 20ms 缓动 + 到位/限位提示音。

## 字库

中文只提取代码里用到的 19 个字符。改提示语后执行：

```powershell
python tools/gen_font.py --preview docs/ui-preview.png
```

## 串口调试输出

```text
=== Project 04: STM32 Servo Angle Control ===
[SYS] HCLK=72000000 Hz
[TOUCH] idle high=0/50 -> active level HIGH
[CFG] start=90 deg step=5 deg ramp=2 deg/20ms
[OLED] ok, I2C2 addr=0x3C
[TARGET] 120
[MODE] PRESET
[PRESET] 3 (90 deg)
[MODE] HOME 90
```

- `[TARGET] xx`：目标角度变化
- `[MODE] MANUAL/PRESET/SWEEP`：模式切换
- `[PRESET] n (xx deg)`：预设点切换
