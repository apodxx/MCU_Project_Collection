# 25 个项目清单 · 杂志风图文 来源记录

- 作品：《单片机从0到100》25 个项目总览，5 页，1080×1440
- 版式：editorial-magazine 技能视觉语言（封面 + 分级索引），按“只列名称”的需求把四种子版式改成索引页
- 主题：editorial / technology，accent `#243E5C`，paper `#F0EEE5`，ink `#27313B`
- 字体：系统回退（Noto Serif SC → 宋体），导出 `fontResolution` 记为 fallback / system

## 文字来源

根目录 `README.md` 的项目总览表（25 行），名称、主控平台、难度等级逐条照录，未改写、未增补。

## 图片来源

封面 5×5 编号方格为脚本绘制（`build_index.py`），不涉及外部图片。

## 制作

- `build_index.py`：生成编号方格、拼装 5 页 HTML（内嵌 PNG）
- `export.py`（editorial-magazine）+ 系统 Edge + `--font-mode system`
- 逐页检查：无溢出、无遮挡，页码 01–05 一致
