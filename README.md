# 单片机从 0 到 100 · MCU 项目合集

> 用一套不到 100 元的模块，从零基础做到完整项目。
> 接线图纸、完整源码、项目资料全部开源，持续更新。

本仓库收录 **25 个嵌入式实战项目**，覆盖 STM32F103C8T6 与 ESP32-S3 两个平台，
按难度分为 5 档，从外设驱动一路做到物联网系统、算法识别与综合毕设。

## 项目总览

| # | 项目 | 主控 | 难度 | 状态 |
| :-: | :-- | :-- | :-: | :-: |
| 01 | [基于 STM32 的智能温湿度监测系统](01_stm32_temp_humidity_monitor/README.md) | STM32F103C8T6 | ★ | ✅ |
| 02 | [基于 STM32 的智能光照控制系统](02_stm32_light_control_system/README.md) | STM32F103C8T6 | ★ | ⬜ |
| 03 | [基于 STM32 的智能声音检测与报警系统](03_stm32_sound_detection_alarm/README.md) | STM32F103C8T6 | ★ | ⬜ |
| 04 | [基于 STM32 的智能舵机角度控制系统](04_stm32_servo_angle_control/README.md) | STM32F103C8T6 | ★ | ⬜ |
| 05 | [基于 STM32 的智能防盗报警系统](05_stm32_anti_theft_alarm/README.md) | STM32F103C8T6 | ★ | ⬜ |
| 06 | [基于 STM32 的多功能电子数据采集终端](06_stm32_multi_sensor_daq/README.md) | STM32F103C8T6 | ★ | ⬜ |
| 07 | [基于 STM32 的智能宿舍环境监测系统](07_stm32_dorm_env_monitor/README.md) | STM32F103C8T6 | ★★ | ⬜ |
| 08 | [基于 STM32 的智能教室环境监测与照明控制系统](08_stm32_classroom_env_lighting/README.md) | STM32F103C8T6 | ★★ | ⬜ |
| 09 | [基于 ESP32-S3 的智能植物养护监测系统](09_esp32s3_plant_care_monitor/README.md) | ESP32-S3 | ★★ | ⬜ |
| 10 | [基于 STM32 的智能仓储环境监测系统](10_stm32_warehouse_env_monitor/README.md) | STM32F103C8T6 | ★★ | ⬜ |
| 11 | [基于 ESP32-S3 的智能家居控制终端](11_esp32s3_smart_home_terminal/README.md) | ESP32-S3 | ★★ | ⬜ |
| 12 | [基于 ESP32-S3 的智能环境报警与远程通知系统](12_esp32s3_env_alarm_notifier/README.md) | ESP32-S3 | ★★ | ⬜ |
| 13 | [基于 ESP32-S3 的物联网智能环境监测平台](13_esp32s3_iot_env_platform/README.md) | ESP32-S3 | ★★★ | ⬜ |
| 14 | [基于 ESP32-S3 的智能家居物联网管理系统](14_esp32s3_smart_home_iot/README.md) | ESP32-S3 | ★★★ | ⬜ |
| 15 | [基于 STM32 与 ESP8266 的无线传感器数据采集系统](15_stm32_esp8266_wireless_daq/README.md) | STM32F103C8T6 + ESP8266 | ★★★ | ⬜ |
| 16 | [基于 ESP32-S3 的智能设备远程运维系统](16_esp32s3_device_remote_ops/README.md) | ESP32-S3 | ★★★ | ⬜ |
| 17 | [基于 ESP32-S3 的多传感器智能安防监测系统](17_esp32s3_multi_sensor_security/README.md) | ESP32-S3 | ★★★ | ⬜ |
| 18 | [基于 MPU6050 的智能跌倒检测与报警系统](18_esp32s3_fall_detection_alarm/README.md) | ESP32-S3 | ★★★★ | ⬜ |
| 19 | [基于 STM32 的智能设备振动监测与故障预警系统](19_stm32_vibration_fault_warning/README.md) | STM32F103C8T6 | ★★★★ | ⬜ |
| 20 | [基于 ESP32-S3 的多传感器融合环境异常识别系统](20_esp32s3_sensor_fusion_anomaly/README.md) | ESP32-S3 | ★★★★ | ⬜ |
| 21 | [基于 ESP32-S3 的智能人员活动状态监测系统](21_esp32s3_activity_monitor/README.md) | ESP32-S3 | ★★★★ | ⬜ |
| 22 | [基于 ESP32-S3 的智能养老居室监护系统](22_esp32s3_elderly_care_room/README.md) | ESP32-S3 | ★★★★★ | ⬜ |
| 23 | [基于 ESP32-S3 的智能老人健康数据采集与管理终端](23_esp32s3_elderly_health_terminal/README.md) | ESP32-S3 | ★★★★ | ⬜ |
| 24 | [基于 ESP32-S3 的智能养老陪护交互终端](24_esp32s3_elderly_companion_terminal/README.md) | ESP32-S3 | ★★★★★ | ⬜ |
| 25 | [基于 ESP32-S3 的智能移动机器人环境感知与交互控制系统](25_esp32s3_mobile_robot/README.md) | ESP32-S3 | ★★★★★ | ⬜ |

## 难度分级

- **★ 入门级**：外设驱动、数据采集、显示与简单控制
- **★★ 基础综合型**：完整功能系统，适合做实物演示
- **★★★ 物联网应用型**：硬件 + 网络 + 前后端，展示综合能力
- **★★★★ 算法与智能检测型**：滤波、融合、识别，论文有技术内容
- **★★★★★ 综合创新型**：完整系统，完成度与展示度最高

> 想做毕设、时间有限？优先看 **22（养老居室监护）**、**18（跌倒检测）**、**14（智能家居 IoT）**、
> **19（振动故障预警）**、**20（多传感器融合）**、**25（移动机器人）**。

## 硬件清单

| 类别 | 模块 | 主要用途 |
| :-- | :-- | :-- |
| 主控 | STM32F103C8T6 核心板 | 外设驱动、ADC、PWM、I2C、SPI |
| 主控 | ESP32-S3 开发板 | Wi-Fi 联网、物联网、算法与综合项目 |
| 显示 | 0.96" OLED（I2C） | 数据显示与菜单交互 |
| 传感器 | DHT11 | 温湿度采集 |
| 传感器 | 光敏电阻模块 | 光照采集 |
| 传感器 | MQ 气体传感器 | 烟雾 / 可燃气体检测 |
| 传感器 | 麦克风模块 | 声音强度检测 |
| 传感器 | MPU6050 | 六轴姿态 / 振动检测 |
| 传感器 | 红外避障模块 | 近距物体 / 人员检测 |
| 传感器 | 槽型光电传感器 | 轮速 / 计数 |
| 输入 | 触摸模块 | 触摸按键 |
| 输入 | 旋转编码器（EC11） | 调参、菜单 |
| 执行 | 继电器模块 | 照明 / 家电控制 |
| 执行 | 蜂鸣器 | 报警 / 提示 |
| 执行 | SG90 舵机 | 角度 / 动作控制 |
| 存储 | W25QXX SPI Flash | 历史数据、日志、缓存 |
| 通信 | ESP8266 | Wi-Fi（AT 指令 / UART） |

> 待确认硬件：红色生物传感器模块（心率 / 血氧），型号未确认，相关项目暂不将其作为必需硬件。

## 目录规范

每个项目一个独立目录，命名规则：`编号_主控_项目名`（英文小写，方便工具链与命令行使用）。

```text
.
├─ README.md                      # 项目总览（本文件）
├─ 01_stm32_temp_humidity_monitor/
│  └─ README.md                   # 项目说明：硬件、功能、技术要点
├─ 02_stm32_light_control_system/
│  └─ README.md
├─ ...
└─ 素材/                           # 本地素材，不纳入版本管理
```

项目开始开发时，在该项目目录下补充：

```text
NN_project_name/
├─ firmware/     # 主控工程：Keil MDK / STM32CubeIDE / ESP-IDF
├─ hardware/     # 接线图、原理图、BOM
└─ docs/         # 流程图、测试记录、演示材料
```

## 开发环境

- **STM32**：STM32CubeMX + Keil MDK-ARM 5 / STM32CubeIDE，ST-Link 下载调试
- **ESP32-S3**：ESP-IDF v5.x（或 Arduino-ESP32），USB 串口下载
- **调试工具**：串口助手（XCOM）、逻辑分析仪（可选）

> 新手从零搭建 STM32 环境（CubeIDE + CubeMX 下载安装、建工程、编译烧录、常见问题）：
> [环境搭建-STM32CubeIDE](docs/环境搭建-STM32CubeIDE.md)

## 学习路线

| 阶段 | 项目 | 目标 |
| :-- | :-- | :-- |
| 第一阶段 · 入门 | 01 – 06 | 外设驱动、ADC、PWM、I2C、串口 |
| 第二阶段 · 基础综合 | 07 – 12 | 完整功能系统、多传感器与本地交互 |
| 第三阶段 · 物联网 | 13 – 17 | 数据协议、断线重连、云端与 Web 前端 |
| 第四阶段 · 算法 | 18 – 21 | 滤波、融合、跌倒 / 振动 / 活动识别 |
| 第五阶段 · 综合创新 | 22 – 25 | 完整系统设计与答辩级作品 |

## 状态说明

- ⬜ 未开始　-　🚧 开发中　-　✅ 已完成

## 说明

- 选题来源于《识别模块清单》整理的 25 个本科毕设课题，按硬件复用与难度沿用原分级。
- 所有项目均基于现有模块设计，尽量不额外增加硬件成本；个别项目需要的扩展硬件已在备注中注明。
- 项目 01 已完成：固件、接线图、接线说明、界面预览齐全，可直接复现；其余项目随进度逐个补齐。
