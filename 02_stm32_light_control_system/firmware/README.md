# 固件工程说明（STM32CubeIDE）

STM32F103C8T6 · 72MHz · HAL 库 · STM32CubeIDE 2.1.1

## 目录结构

```
firmware/
├─ 02_stm32_light_control_system.ioc  # CubeMX 配置（引脚、时钟、外设）
├─ Core/
│  ├─ Inc/
│  │  ├─ board.h        # 引脚定义、极性开关、阈值参数
│  │  ├─ adc.h          # ADC 采集
│  │  ├─ oled.h         # OLED 驱动
│  │  ├─ oled_font.h    # 字库声明
│  │  └─ app.h
│  ├─ Src/
│  │  ├─ main.c         # CubeMX 生成（只在 USER CODE 区加了 3 行）
│  │  ├─ app.c          # 业务逻辑：采光 / 阈值 / 继电器 / 触摸 / 界面
│  │  ├─ adc.c          # ADC1_IN0 初始化与单次转换
│  │  ├─ oled.c         # SSD1306 显存与绘制（复用项目 01）
│  │  ├─ oled_font_ascii.c / oled_font_cn.c  # 字库（脚本生成）
│  │  ├─ retarget.c     # printf 重定向到 USART1
│  │  └─ gpio.c / i2c.c / usart.c  # CubeMX 生成
│  └─ Startup/
├─ Drivers/             # CMSIS + STM32F1 HAL（含手工加入的 ADC 驱动）
├─ tools/gen_font.py    # 字库生成 / 界面预览
└─ STM32F103C8TX_FLASH.ld
```

## 引脚

| 功能 | 引脚 | 说明 |
| :-- | :-- | :-- |
| 光敏电阻 AO | PA0 | ADC1_IN0，模拟输入 |
| 触摸模块 OUT | PA3 | 输入（下拉），极性上电自检 |
| 继电器 IN | PB5 | 输出，默认低电平触发 |
| OLED SCL / SDA | PB10 / PB11 | I2C2（PB6/PB7 的 I2C1 也可，自动识别） |
| 调试串口 | PA9 / PA10 | USART1，115200 |
| ST-Link | PA13 / PA14 | SWDIO / SWCLK |
| 板载 LED | PC13 | 低电平点亮，心跳指示 |

## 关于 ADC

`.ioc` 里只把 **PA0 配成了模拟输入**，ADC1 的 IP 配置没有写进去；
`adc.c` 里手工完成时钟使能、`HAL_ADC_Init`、通道配置和校准。
`stm32f1xx_hal_conf.h` 里的 `HAL_ADC_MODULE_ENABLED` 已打开，
`Drivers/STM32F1xx_HAL_Driver` 下补了 `stm32f1xx_hal_adc.c / _ex.c`。

## 编译 / 烧录

```text
File > Open Projects from File System... -> 选本目录 -> Ctrl+B -> ▶ 烧录
```

## 代码结构说明

- **main.c 只改了三处**：`USER CODE Includes` 加 `app.h`，`USER CODE 2` 调 `app_init()`，
  `USER CODE 3` 调 `app_task()`，CubeMX 重新生成代码不会覆盖。
- **adc.c**：单次转换 + 软件平均（app 里 8 次求平均），每 100ms 采一次。
- **app.c**：自动模式走「阈值 + 回差」，手动模式由触摸键直接开关继电器；
  短按/长按按页面分工，所有逻辑在 `app_task()` 轮询。

## 字库

中文只提取代码里用到的 19 个字。改提示语后执行：

```powershell
python tools/gen_font.py --preview docs/ui-preview.png
```

## 串口调试输出

```text
=== Project 02: STM32 Light Control ===
[SYS] HCLK=72000000 Hz
[TOUCH] idle high=0/50 -> active level HIGH
[CFG] threshold=30% step=5% hysteresis=3%
[OLED] ok, I2C2 addr=0x3C
[RELAY] ON
[MODE] MANUAL
[TH] 35%
```

- `[RELAY] ON/OFF`：继电器动作
- `[MODE] AUTO/MANUAL`：模式切换
- `[TH] xx%`：阈值调整
