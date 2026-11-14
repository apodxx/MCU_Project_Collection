# 02 · 基于 STM32 的智能光照控制系统

> **难度** ★　|　**主控** STM32F103C8T6　|　**状态** 🚧 代码已完成，待上机验证

用光敏电阻采集环境亮度，**旋转编码器**设定阈值，低于阈值自动点亮指示灯（LED）；
触摸键负责自动/手动切换，手动模式下长按直接开关灯。

![界面预览](docs/ui-preview.png)

> 图为按固件同款字库渲染的 128×64 界面示意（`firmware/tools/gen_font.py --preview` 生成）。

## 功能

- ADC 采集光敏电阻，8 次平均滤波，换算成 0~100% 亮度
- **自动模式**：亮度 ≤ 阈值 → 点亮 LED；亮度 ≥ 阈值 + 3% → 熄灭（回差防抖）
- **编码器旋钮调阈值**：5%~95%，每格 5%，屏幕上实时显示
- **触摸键**：短按切自动/手动；手动模式下长按开灯/关灯
- 主界面底部提示每 3 秒轮换（「旋钮调阈值」↔「短按切手动 / 长按开关灯」）
- 串口（USART1，115200）输出模式、阈值、开关灯日志

> 手头暂时没有继电器模块，执行器先用 **LED** 代替；
> 以后买到继电器，把 `board.h` 里的 `ACTUATOR_MODE` 改成 `ACTUATOR_RELAY` 即可，引脚不用动。

## 硬件清单

| 模块 | 数量 | 说明 |
| :-- | :-- | :-- |
| STM32F103C8T6 最小系统板 | 1 | 蓝色核心板，8MHz 晶振 |
| 光敏电阻模块 | 1 | 3/4 针，用 AO 模拟输出 |
| 旋转编码器（360°，A/C/B 三脚） | 1 | 无按键版本，A/B 接 PA1/PA2，C 接 GND |
| 0.96" OLED（SSD1306，I2C） | 1 | 4 针：VCC/GND/SCL/SDA |
| 触摸模块（TTP223） | 1 | 复用项目 01 的模块 |
| LED（任意颜色）+ 330Ω 电阻 | 1 | 指示灯，代替继电器 |
| ST-Link 下载器 | 1 | 烧录 + 调试 |

## 接线

![接线图](hardware/wiring.png)

完整接线步骤、光敏方向判断、上电检查清单：**[hardware/接线说明.md](hardware/接线说明.md)**

| 模块 | 模块引脚 | 接到 STM32 | 说明 |
| :-- | :-- | :-- | :-- |
| 光敏电阻 | VCC / GND | 3.3V / GND | 不要接 5V |
| 光敏电阻 | AO | **PA0** | ADC1_IN0，模拟量 |
| 旋转编码器 | A | **PA1** | 内部上拉输入 |
| 旋转编码器 | B | **PA2** | 内部上拉输入 |
| 旋转编码器 | C（公共端） | **GND** | 三脚编码器的中间脚 |
| OLED | VCC / GND | 3.3V / GND | |
| OLED | SCL / SDA | **PB10 / PB11** | I2C2；接 PB6/PB7 也能自动识别 |
| 触摸模块 | VCC / GND | 3.3V / GND | TTP223 |
| 触摸模块 | OUT | **PA3** | 高/低有效上电自动识别 |
| LED | 正极（长脚） | **PB5 → 330Ω** | 电阻串在 PB5 和 LED 之间 |
| LED | 负极（短脚） | GND | |
| ST-Link | SWDIO / SWCLK | PA13 / PA14 | 另接 3.3V、GND |

> 面包板上：板子 `3V3` → **+ 轨**、`GND` → **- 轨**；模块 `VCC` → **+ 轨**、`GND` → **- 轨**。
> 编码器和 LED 只需要接 GND，不接 3V3。

## 操作说明

| 操作 | 自动模式 | 手动模式 |
| :-- | :-- | :-- |
| 旋转编码器 | 调阈值（±5%，实时） | 调阈值（不影响当前灯状态） |
| 短按触摸键（< 0.8s） | 切到手动 | 切到自动 |
| 长按触摸键（≥ 0.8s） | 无动作 | 点亮 / 熄灭 LED |

- 阈值范围 5%~95%，转到 95 再转回到 5
- 切回自动模式会立即按当前光照重新判断一次，不会停留在旧状态
- 上电后 200ms 内固件在识别触摸模块极性，这时**不要碰触摸板**

## 软件设计

```
main.c ──> app_init()
            ├─ LED / 执行器初始化（默认关闭）
            ├─ 触摸引脚初始化（PA3 下拉）+ 极性自检
            ├─ encoder_init()   PA1/PA2 上拉输入，正交解码
            ├─ adc_init()       ADC1 初始化、校准（PA0）
            └─ oled_init()      I2C1/I2C2、0x3C/0x3D 自动探测

while(1) ─> app_task()
            ├─ button_poll()    20ms 消抖 + 短按/长按
            ├─ encoder_poll()   正交解码，累计 4 个跳变算一格
            ├─ sensor_poll()    每 100ms 采 8 次 ADC 取平均
            ├─ apply_auto_control()  阈值 + 回差判断
            └─ ui_task()        200ms 刷屏
```

几个实现要点：

- **编码器解码**：A/B 两相组成 4 个状态，用 16 项状态表判断正转/反转/抖动，
  每 4 个有效跳变算一格（`ENC_COUNTS_PER_STEP`）；方向反了改 `ENC_REVERSE` 或对调 A/B。
- **ADC**：`ADC1_IN0` 单次转换 + 8 次平均；ADC 在 `adc.c` 里手工初始化
  （`.ioc` 里只把 PA0 配成了模拟输入）。
- **回差控制**：开灯用 `≤ 阈值`，关灯用 `≥ 阈值 + 3%`，中间区间保持状态。
- **执行器可切换**：`board.h` 里 `ACTUATOR_MODE` 可选 `ACTUATOR_LED / ACTUATOR_RELAY / ACTUATOR_BUZZER`。
- **触摸极性**：TTP223 有高有效/低有效两种，上电采样 200ms 空闲电平自动判断。

## 编译与烧录

环境搭建（CubeIDE 2.1.1 + CubeMX）见 [环境搭建文档](../docs/环境搭建-STM32CubeIDE.md)。

1. 打开 STM32CubeIDE：`File > Open Projects from File System...`
2. 目录选择本项目的 `firmware` 文件夹，Finish
3. `Ctrl+B` 编译（当前占用：Flash 约 26KB / 64KB，RAM 约 3.3KB / 20KB）
4. 接好 ST-Link，点 ▶ 烧录

## 调试提示

| 现象 | 排查方向 |
| :-- | :-- |
| 光照一直是 0% 或 100% | AO 没接 PA0、模块没供电；用万用表量 AO 电压应在 0~3.3V 变化 |
| 光照数值和明暗相反 | 改 `board.h` 的 `LIGHT_DARK_WHEN_LOW`（1/0 互换），重新编译 |
| 转编码器阈值不动 | A/B 没接 PA1/PA2、C 没接 GND；量 A/B 对地电压，转动时应在 0/3.3V 跳变 |
| 转一格跳两下 | `ENC_COUNTS_PER_STEP` 改成 8 |
| 转好几格才动一下 | `ENC_COUNTS_PER_STEP` 改成 2 |
| 旋转方向反了 | `ENC_REVERSE` 改成 1，或对调 A/B 两根线 |
| LED 不亮 | LED 正负极接反（长脚是正）、330Ω 虚接、或 PB5 没输出 |
| 触摸没反应 | OUT 没接 PA3；上电时别碰触摸板，否则极性会识别反 |
| 屏幕黑屏 | 同项目 01：查 VCC/GND/SCL/SDA，固件会自动试 I2C1/I2C2 |

## 关于继电器

现在用 LED 只是**代替执行器**跑通逻辑，项目本身仍然是「光照控制照明」。
买到单路继电器模块后：

1. `board.h` 把 `ACTUATOR_MODE` 改成 `ACTUATOR_RELAY`
2. PB5 接继电器 IN，继电器 VCC/GND 接 3V3/GND
3. 重新编译烧录即可，其他代码不用动

⚠️ 继电器接 220V 灯具时：先用 5V 小灯测试逻辑，接市电必须断电操作、做好绝缘。

## 彩蛋：达文西的手电筒（有光才亮）

> 《国产凌凌漆》里达文西的「太阳能手电筒」：**有光的时候它就会亮，没有光的时候绝对不亮**。
> 本项目只要把自动模式的判断反过来，就能复刻这个效果。下面只写方法，不改默认代码。

### 现在的逻辑（正常照明）

```c
/* app.c -> apply_auto_control() */
if (s_light <= s_threshold)                        set_lamp(1);   /* 暗  -> 开灯 */
else if (s_light >= s_threshold + APP_HYSTERESIS)  set_lamp(0);   /* 亮  -> 关灯 */
```

### 达文西逻辑（有光才亮）

把比较方向对调，回差方向也要跟着对调：

```c
/* 有光才亮：亮 -> 开灯；暗 -> 关灯 */
if (s_light >= s_threshold)                        set_lamp(1);
else if (s_light <= s_threshold - APP_HYSTERESIS)  set_lamp(0);
```

改完重新编译烧录：手电照它 → LED 亮；一遮住 → LED 灭。

### 想做成可切换的

在 `board.h` 加一个宏：

```c
#define LAMP_DAYLIGHT_MODE  1   /* 1 = 达文西模式，0 = 正常照明 */
```

`apply_auto_control()` 里改成：

```c
#if (LAMP_DAYLIGHT_MODE != 0)
  if (s_light >= s_threshold)                        set_lamp(1);
  else if (s_light <= s_threshold - APP_HYSTERESIS)  set_lamp(0);
#else
  if (s_light <= s_threshold)                        set_lamp(1);
  else if (s_light >= s_threshold + APP_HYSTERESIS)  set_lamp(0);
#endif
```

### 几个注意点

- `s_threshold` 最小 5、`APP_HYSTERESIS` 是 3，`s_threshold - 3` 最小是 2，不会出现无符号下溢。
- 手动模式和触摸键不受影响：短按切模式、长按手动开关灯，照旧。
- 屏幕上的「光照」和「阈值」含义不变，只是灯的判断方向反了。
- 这个模式适合演示、或者做「有光时亮起」的指示；真正的照明控制还是用默认逻辑。

## 扩展方向

- 把 LED 换成继电器 + 台灯，做真正的照明控制
- 加人体红外（PIR，你已有），实现「有人且光线暗」才开灯
- 用 W25QXX 记录光照曲线
- 加 ESP8266 上传光照数据（见项目 15）

## 进度

- [x] 硬件选型与接线方案（光敏 + 编码器 + 触摸 + LED）
- [x] CubeMX 工程（72MHz、I2C1/I2C2、USART1、GPIO）
- [x] ADC 采集与滤波
- [x] 编码器正交解码 + 阈值调整
- [x] 触摸键交互（短按切模式 / 长按开关灯）
- [x] 执行器可切换（LED / 继电器 / 蜂鸣器）
- [x] 中文界面与界面预览
- [x] 编译通过（0 error / 0 warning）
- [ ] 上机联调
