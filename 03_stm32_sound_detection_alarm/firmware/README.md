# 固件工程说明（STM32CubeIDE）

STM32F103C8T6 · 72MHz · HAL 库 · STM32CubeIDE 2.1.1

## 目录结构

```
firmware/
├─ 03_stm32_sound_detection_alarm.ioc  # CubeMX 配置（引脚、时钟、外设）
├─ Core/
│  ├─ Inc/
│  │  ├─ board.h        # 引脚定义、极性宏、阈值范围
│  │  ├─ adc.h          # ADC 采集
│  │  ├─ encoder.h      # 旋转编码器正交解码
│  │  ├─ oled.h         # OLED 驱动
│  │  ├─ oled_font.h    # 字库声明
│  │  └─ app.h
│  ├─ Src/
│  │  ├─ main.c         # CubeMX 生成（只在 USER CODE 区加了 3 行）
│  │  ├─ app.c          # 业务逻辑：采声音 / 阈值 / 蜂鸣器 / 界面
│  │  ├─ adc.c          # ADC1 初始化与单次转换
│  │  ├─ encoder.c      # A/B 状态表解码
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
| 声音传感器 AO | PA0 | ADC1_IN0，模拟输入 |
| 编码器 A / B | PA1 / PA2 | 内部上拉输入，正交解码 |
| 蜂鸣器 IN | PB5 | 输出，默认低电平触发 |
| OLED SCL / SDA | PB10 / PB11 | I2C2（PB6/PB7 的 I2C1 也可，自动识别） |
| 调试串口 | PA9 / PA10 | USART1，115200 |
| ST-Link | PA13 / PA14 | SWDIO / SWCLK |
| 板载 LED | PC13 | 低电平点亮，心跳指示 |

## 声音输出类型（重要）

`board.h` 里根据模块类型二选一：

```c
#define SOUND_OUT_MODE   SOUND_OUT_ANALOG   /* 默认：OUT 是模拟电压 */
// #define SOUND_OUT_MODE   SOUND_OUT_DIGITAL /* OUT 只有高低电平 */
```

- **模拟输出**：数值 0~100% 连续变化，阈值判断正常。
- **数字输出**：模块内部比较器输出 0/3.3V，固件把「高电平」当作 100%，
  灵敏度由模块上的蓝色电位器决定；如果数值只在 0/100 跳，就改成数字模式。

## 关于 ADC

`.ioc` 里只把 **PA0 配成模拟输入**，ADC1 的 IP 配置没有写进去；
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
- **adc.c**：单次转换，app 里每 20ms 采 4 次取平均。
- **encoder.c**：16 项状态表解码，`ENC_COUNTS_PER_STEP` 控制每格跳变数，`ENC_REVERSE` 控制方向。
- **app.c**：快起慢落平滑 → 阈值 + 回差判断 → 报警计数 → 蜂鸣器节拍；
  编码器转一格阈值 ±5%。

## 字库

中文只提取代码里用到的 13 个字。改提示语后执行：

```powershell
python tools/gen_font.py --preview docs/ui-preview.png
```

## 串口调试输出

```text
=== Project 03: STM32 Sound Detection & Alarm ===
[SYS] HCLK=72000000 Hz
[CFG] threshold=60% step=5% hysteresis=3%
[OLED] ok, I2C2 addr=0x3C
[TH] 65%
[ALARM] count=1 level=78%
[ALARM] clear
```

- `[TH] xx%`：编码器调阈值
- `[ALARM] count=n level=xx%`：触发报警并计数
- `[ALARM] clear`：声音回落、报警解除
