# 杂志图文 · 来源与制作记录

- 作品：项目 04（自动闸门控制系统）编辑专题，5 页，1080×1440
- 版式：editorial-magazine 技能 · explainer（四页编辑拼贴）扩展为 5 页
- 主题配色：editorial / technology，seed 3593185471，palette navy
  - accent `#243E5C`，paper `#F0EEE5`，ink `#27313B`，onAccent `#FFF8ED`
- 字体：系统回退（Noto Serif SC → 宋体），导出报告 `fontResolution` 记录为 fallback / system

## 文字来源

全部来自本仓库项目 04 的实现与其文档，未引用外部资料：
- `04_stm32_servo_angle_control/README.md`（功能、接线、调试、进度）
- `04_stm32_servo_angle_control/hardware/接线说明.md`（引脚与电源）
- `04_stm32_servo_angle_control/firmware/Core/Inc/board.h`（阈值、延时、消抖参数）
- 固件编译结果：text 25.3KB / bss 3.3KB

## 图片来源

均为项目自制图，非外部素材：
- `hardware/gate-demo.png` —— 闸门效果示意图（关闸 / 开闸 / 红外与车）
- `docs/ui-preview.png` —— OLED 界面预览（自动 / 手动等待 / 运动）
- `hardware/wiring.png` —— 接线图（含红外 OUT→PA4 局部）

图片经裁剪、拼贴后以内嵌 data URI 形式写入 `magazine.html`，可离线打开。

## 制作

- `build_magazine.py`：裁剪素材、拼贴三联图/竖版概念图、生成内嵌图片的 HTML
- 导出：`export.py`（editorial-magazine 技能）+ 系统 Edge + `--font-mode system`
- 逐页人工检查：原尺寸与 375px 预览，确认无溢出、无遮挡、图注对应
