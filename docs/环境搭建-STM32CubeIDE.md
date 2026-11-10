# STM32 开发环境搭建（STM32CubeIDE + STM32CubeMX）

> 适用：本仓库全部 STM32F103C8T6 项目
> 参考：[波特律动文档站 · 安装 CubeIDE](https://docs.baud-dance.com/docs/stm32/getting-started/getting-started_InstalCube)

## 1. 下载安装包

打开上面的页面，页面里有三个选择，**推荐选「选择二」**：

| 按钮 | 说明 |
| :-- | :-- |
| 下载 CubeIDE 2.1.1 | 集成开发环境（编辑器 + 编译器 + 调试器） |
| 下载 CubeMX | 图形化配置工具。CubeIDE 2.0 之后**不再内置** CubeMX，需要单独装 |

「选择一」的 1.19.0 需要登录 ST 账号才能下载代码，新版本已经不需要登录，直接选二更省事。

> 也可以从 ST 官网下载，但国内访问 ST 官网经常很慢，用上面的镜像按钮更快。

## 2. 安装

两个安装包都**一路下一步**即可，建议都装在默认路径：

- CubeIDE：`D:\ST\STM32CubeIDE_2.1.1`
- CubeMX：`D:\STM32CubeMX`

安装 CubeIDE 时会顺带装好 ST-Link 驱动；如果后面电脑识别不到下载器，再单独装一次 ST-Link 驱动。

## 3. 新建工程（从零）

1. 打开 CubeIDE，`File > New > STM32 Project`
2. 在 **MCU/MPU Selector** 里搜索 `STM32F103C8`，选中 **STM32F103C8T6**（LQFP48），Next
3. 填工程名（例如 `01_stm32_temp_humidity_monitor`），Toolchain 选 **STM32CubeIDE**，Finish
4. 在弹出的配置界面里设置：
   - **SYS** → Debug：`Serial Wire`（一定要设，否则 ST-Link 可能连不上）
   - **RCC** → HSE：`Crystal/Ceramic Resonator`（蓝色核心板上有 8MHz 晶振）
   - **Clock Configuration**：HCLK 拉到 **72MHz**，回车让软件自动算分频
5. 配置外设引脚（本项目）：

   | 外设 | 引脚 | 模式 |
   | :-- | :-- | :-- |
   | I2C1 | PB6 / PB7 | I2C，Fast Mode 400kHz |
   | USART1 | PA9 / PA10 | Asynchronous，115200 8N1 |
   | GPIO | PA1 | Input（DHT11） |
   | GPIO | PA2 | Input（触摸） |
   | GPIO | PB5 | Output（蜂鸣器） |

6. `Project Manager` 页：确认 Toolchain 是 `STM32CubeIDE`，勾选 `Generate Under Root`，点 **GENERATE CODE**
7. 点工具栏的 🔨 编译，第一次会编译整个 HAL，大约 10~30 秒

## 4. 编译与烧录

- **编译**：工具栏锤子图标，或 `Project > Build Project`（快捷键 Ctrl+B）
- **烧录**：板子接 ST-Link（SWDIO→PA13、SWCLK→PA14、3V3、GND），点绿色 ▶ Run
  第一次会弹出调试配置，选 `STM32 C/C++ Application`，默认参数即可
- **串口**：PA9/PA10 接 USB-TTL，115200 8N1，可以看到程序打印

## 5. 直接使用本仓库的工程

仓库里每个项目的 `firmware/` 目录都是**完整的 CubeIDE 工程**（含 `.ioc`、HAL 库、启动文件），不用重新配置：

1. `File > Open Projects from File System...`
2. Directory 选择 `01_.../firmware`，Finish
3. 直接编译、烧录即可

想改引脚/加外设时，双击 `firmware/*.ioc` 打开配置界面，改完 `GENERATE CODE`，自己写的代码放在 `USER CODE BEGIN/END` 之间就不会被覆盖。

## 6. 常见问题

| 现象 | 原因 / 解决 |
| :-- | :-- |
| 编译报 `arm-none-eabi-gcc not found` | CubeIDE 没装全，重装或检查杀毒软件是否拦截 |
| 找不到设备 / 下载失败 | ① 没接 SWDIO/SWCLK；② SYS Debug 没设 `Serial Wire`；③ 板子没供电；④ 用 ST-Link Utility 全片擦除再试 |
| ST-Link 固件过旧 | 打开 STM32CubeProgrammer 提示升级时点升级 |
| 程序下载后不运行 | 检查 Boot0 是否为 0（蓝色板子跳线帽在 0 侧） |
| 串口乱码 | 波特率不是 115200，或 GND 没共地 |
| 选错芯片 | 本项目就踩过：工程建成了 STM32C051C8T6（STM32C0 系列），代码完全不能用在 F103 上。**认准 STM32F103C8T6** |
