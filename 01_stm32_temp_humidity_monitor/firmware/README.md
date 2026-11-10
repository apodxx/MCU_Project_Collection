# 固件工程说明（STM32CubeIDE）

STM32F103C8T6 · 72MHz · HAL 库 · STM32CubeIDE 2.1.1

## 目录结构

```
firmware/
├─ 01_stm32_temp_humidity_monitor.ioc   # CubeMX 配置（引脚、时钟、外设）
├─ Core/
│  ├─ Inc/
│  │  ├─ board.h          # 引脚定义、阈值默认值、硬件开关
│  │  ├─ dht11.h          # DHT11 驱动接口
│  │  ├─ oled.h           # OLED 接口
│  │  ├─ oled_font.h      # 字库声明
│  │  └─ app.h            # 应用层接口
│  ├─ Src/
│  │  ├─ main.c           # CubeMX 生成（只在 USER CODE 区加了 3 行）
│  │  ├─ app.c            # 业务逻辑：采集 / 报警 / 按键 / 界面
│  │  ├─ dht11.c          # 单总线时序驱动（DWT 微秒延时）
│  │  ├─ oled.c           # SSD1306 显存与绘制
│  │  ├─ oled_font_ascii.c# 8x16 ASCII 字库（脚本生成）
│  │  ├─ oled_font_cn.c   # 16x16 中文字库子集（脚本生成）
│  │  ├─ retarget.c       # printf 重定向到 USART1
│  │  └─ gpio.c / i2c.c / usart.c   # CubeMX 生成的初始化
│  └─ Startup/            # 启动文件
├─ Drivers/               # CMSIS + STM32F1 HAL
├─ tools/gen_font.py      # 字库生成 / 界面预览脚本
└─ STM32F103C8TX_FLASH.ld # 链接脚本（64KB Flash / 20KB RAM）
```

## 引脚

| 功能 | 引脚 | 说明 |
| :-- | :-- | :-- |
| DHT11 DATA | PA1 | 开漏输出 + 内部上拉，单总线 |
| 触摸模块 OUT | PA2 | 输入，下拉，高电平有效 |
| 蜂鸣器 IN | PB5 | 输出，默认低电平触发 |
| OLED SCL | PB10（或 PB6） | I2C2_SCL（备选 I2C1），400kHz |
| OLED SDA | PB11（或 PB7） | I2C2_SDA（备选 I2C1） |
| 调试串口 | PA9 / PA10 | USART1，115200 8N1 |
| ST-Link | PA13 / PA14 | SWDIO / SWCLK |

改引脚只需要改 `.ioc` + `Core/Inc/board.h` 里的宏，驱动和业务代码不用动。

## 编译 / 烧录

```text
File > Open Projects from File System... -> 选本目录 -> 编译（Ctrl+B）-> 烧录（▶）
```

命令行编译（可选）：

```powershell
stm32cubeide.exe -nosplash -application org.eclipse.cdt.managedbuilder.core.headlessbuild `
  -data <临时 workspace> -import <本目录> -build 01_stm32_temp_humidity_monitor
```

## 代码结构说明

- **main.c 只改了三处**：`USER CODE Includes` 加 `app.h`，`USER CODE 2` 调 `app_init()`，`USER CODE 3` 调 `app_task()`。
  这样以后在 CubeMX 里改配置重新生成代码，自己的代码不会被覆盖。
- **dht11.c**：用 DWT 周期计数器做 1us 级延时；起始信号用 `HAL_Delay(20)` 不关中断，
  读 40 bit 时才关中断（约 3~5ms），尽量不影响串口和系统节拍。
- **oled.c**：1KB 显存，按页刷屏；每页首字节固定 `0x40`，刷新时整页一次性发出。
- **app.c**：全部逻辑都在 `app_task()` 里轮询，没有用中断和 RTOS，方便初学者跟。

## 字库

中文只提取了代码里真正用到的 31 个字（省 Flash）。改中文提示后执行：

```powershell
python tools/gen_font.py --preview docs/ui-preview.png
```

会重新生成 `oled_font_ascii.c` / `oled_font_cn.c`，并输出界面预览图。

## 串口调试输出

上电后 115200 串口会打印自检信息和运行日志：

```text
=== Project 01: STM32 Temp & Humidity Monitor ===
[SYS] HCLK=72000000 Hz, SYSCLK=72000000 Hz
[I2C] I2C1(PB6/PB7):
[I2C] I2C2(PB10/PB11): 0x3C
[OLED] ok, I2C2 addr=0x3C
T_HI=30 T_LO=10 H_HI=80 H_LO=30
[DHT11] T=26C H=58%
[ALARM] count=1
```

- `[I2C]` 会分别扫描两条总线；OLED 接哪条，哪条后面就会出现 `0x3C`（或 `0x3D`）。
- 两条都为空：屏幕没供电 / 没共地 / SCL、SDA 接反 / 接在了别的引脚上。
- OLED 驱动会自动在 I2C1、I2C2 和 0x3C、0x3D 之间组合尝试，`[OLED] ok,` 后面就是命中的总线和地址。
- DHT11 读失败会打印 `rc=` 错误码：-2 无应答（接线/上拉问题），-3 超时，-4 校验错。

## 不接串口时的自检

| 现象 | 含义 |
| :-- | :-- |
| 板载 LED（PC13）快闪 3 次 | 程序已运行 |
| 蜂鸣器 1 声短鸣 | OLED 初始化成功 |
| 蜂鸣器 2 声短鸣 | OLED 没找到（程序仍正常运行） |
| 每秒亮灭各 0.5s | 主循环心跳 |
| 约 5Hz 连续快闪 | `Error_Handler`：时钟/外设初始化失败 |
