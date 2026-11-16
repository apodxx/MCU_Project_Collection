# -*- coding: utf-8 -*-
"""Build a 5-page editorial index of the 25 course projects."""

import base64
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(r"D:\Users\Administrator\Desktop\单片机从0到100")
WORK = ROOT / "docs" / "magazine-projects"
WORK.mkdir(parents=True, exist_ok=True)
ASSETS = WORK / "assets"
ASSETS.mkdir(exist_ok=True)

ACCENT = (36, 62, 92)
PAPER = (240, 238, 229)

# ---------- cover art: 5x5 numbered tiles ----------
tile, gap = 112, 12
W = H = 5 * tile + 4 * gap
img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
d = ImageDraw.Draw(img)
f_num = ImageFont.truetype(r"C:\Windows\Fonts\consolab.ttf", 40)
for i in range(25):
    r, c = divmod(i, 5)
    x = c * (tile + gap)
    y = r * (tile + gap)
    d.rounded_rectangle([x, y, x + tile, y + tile], radius=10, fill=PAPER)
    label = f"{i+1:02d}"
    tw = d.textlength(label, font=f_num)
    d.text((x + (tile - tw) / 2, y + tile / 2 - 26), label, font=f_num, fill=ACCENT)
img.save(ASSETS / "cover-grid.png")

text_uris = {
    "COVER": "data:image/png;base64," + base64.b64encode((ASSETS / "cover-grid.png").read_bytes()).decode(),
}

PROJECTS = [
    ("01", "基于 STM32 的智能温湿度监测系统", "STM32F103C8T6", 1),
    ("02", "基于 STM32 的智能光照控制系统", "STM32F103C8T6", 1),
    ("03", "基于 STM32 的智能声音检测与报警系统", "STM32F103C8T6", 1),
    ("04", "基于 STM32 的自动闸门控制系统", "STM32F103C8T6", 1),
    ("05", "基于 STM32 的智能防盗报警系统", "STM32F103C8T6", 1),
    ("06", "基于 STM32 的多功能电子数据采集终端", "STM32F103C8T6", 1),
    ("07", "基于 STM32 的智能宿舍环境监测系统", "STM32F103C8T6", 2),
    ("08", "基于 STM32 的智能教室环境监测与照明控制系统", "STM32F103C8T6", 2),
    ("09", "基于 ESP32-S3 的智能植物养护监测系统", "ESP32-S3", 2),
    ("10", "基于 STM32 的智能仓储环境监测系统", "STM32F103C8T6", 2),
    ("11", "基于 ESP32-S3 的智能家居控制终端", "ESP32-S3", 2),
    ("12", "基于 ESP32-S3 的智能环境报警与远程通知系统", "ESP32-S3", 2),
    ("13", "基于 ESP32-S3 的物联网智能环境监测平台", "ESP32-S3", 3),
    ("14", "基于 ESP32-S3 的智能家居物联网管理系统", "ESP32-S3", 3),
    ("15", "基于 STM32 与 ESP8266 的无线传感器数据采集系统", "STM32 + ESP8266", 3),
    ("16", "基于 ESP32-S3 的智能设备远程运维系统", "ESP32-S3", 3),
    ("17", "基于 ESP32-S3 的多传感器智能安防监测系统", "ESP32-S3", 3),
    ("18", "基于 MPU6050 的智能跌倒检测与报警系统", "ESP32-S3", 4),
    ("19", "基于 STM32 的智能设备振动监测与故障预警系统", "STM32F103C8T6", 4),
    ("20", "基于 ESP32-S3 的多传感器融合环境异常识别系统", "ESP32-S3", 4),
    ("21", "基于 ESP32-S3 的智能人员活动状态监测系统", "ESP32-S3", 4),
    ("22", "基于 ESP32-S3 的智能养老居室监护系统", "ESP32-S3", 5),
    ("23", "基于 ESP32-S3 的智能老人健康数据采集与管理终端", "ESP32-S3", 4),
    ("24", "基于 ESP32-S3 的智能养老陪护交互终端", "ESP32-S3", 5),
    ("25", "基于 ESP32-S3 的智能移动机器人环境感知与交互控制系统", "ESP32-S3", 5),
]

TIERS = [
    ("入门", "外设驱动、传感器与显示", 1, 6),
    ("进阶", "多模块配合、数据记录与联动", 7, 12),
    ("联网", "Wi-Fi、MQTT、远程控制", 13, 17),
    ("综合", "算法识别、系统集成、毕设级", 18, 25),
]


def stars(n):
    return "★" * n


def page_block(no, title, desc, items, page_no):
    rows = []
    for num, name, plat, tier in items:
        rows.append(
            f'<div class="row"><span class="num">{num}</span>'
            f'<span class="name">{name}</span>'
            f'<span class="meta"><em>{plat}</em><b>{stars(tier)}</b></span></div>'
        )
    return f"""<article data-page class="index-page" aria-label="{title}">
<div class="running"><span>单片机从0到100 · 25 个项目</span><span>{no} / {desc}</span></div>
<h1 class="tier">{title}</h1>
<p class="tier-deck">{desc}　·　共 {len(items)} 个</p>
<div class="rows">
{''.join(rows)}
</div>
<footer><span>单片机从0到100 · 项目总览</span><span>{page_no:02d} / 05</span></footer></article>"""


pages = [
    f"""<article data-page class="cover-page" aria-label="封面：25 个项目">
<div class="running"><span>单片机从0到100</span><span>项目总览</span></div>
<p class="cover-deck">从点灯、传感器，<br>到物联网与移动机器人</p>
<img class="cover-art" src="{{{{COVER}}}}" alt="25 个项目的编号方格">
<h1><span>二十五个</span><span>项目清单</span></h1>
<footer><span>25 个项目 · 5 个难度档</span><span>01 / 05</span></footer></article>""",
]
for i, (title, desc, lo, hi) in enumerate(TIERS):
    items = [p for p in PROJECTS if lo <= int(p[0]) <= hi]
    pages.append(page_block(f"{['壹','贰','叁','肆'][i]} · {title}", title, desc, items, i + 2))

HTML = """<!doctype html><html lang="zh-CN"><head><meta charset="utf-8"><title>25 个项目 · 编辑总览</title><style>
:root{--paper:#F0EEE5;--ink:#27313B;--accent:#243E5C;--on-accent:#FFF8ED;--rule-soft:color-mix(in srgb,var(--accent) 25%,transparent)}
*{box-sizing:border-box}body{margin:0;background:#d4cdc3;color:var(--ink);font-family:"Noto Serif SC","Songti SC","SimSun",serif}
article[data-page]{width:1080px;height:1440px;margin:0 auto 32px;padding:62px;position:relative;background:var(--paper)}
h1,h2,p{margin:0}img{display:block;max-width:100%}
.running{display:flex;justify-content:space-between;font:19px/1.4 "Noto Sans SC","PingFang SC",sans-serif;letter-spacing:2px;border-bottom:1px solid var(--accent);padding-bottom:12px;margin-bottom:30px;color:var(--accent)}
footer{position:absolute;bottom:32px;left:62px;right:62px;display:flex;justify-content:space-between;border-top:1px solid var(--rule-soft);padding-top:11px;font:17px/1.4 "Noto Sans SC","PingFang SC",sans-serif}
.cover-page{background:var(--accent)!important;color:var(--on-accent)}
.cover-page .running{color:var(--on-accent);border-color:color-mix(in srgb,var(--on-accent) 36%,transparent)}
.cover-page footer{color:var(--on-accent);border-color:color-mix(in srgb,var(--on-accent) 36%,transparent)}
.cover-page .cover-art{position:absolute;width:616px;height:616px;right:56px;top:330px;object-fit:contain}
.cover-page .cover-deck{position:absolute;left:65px;top:150px;font-size:24px;line-height:1.7;letter-spacing:2px}
.cover-page h1{position:absolute;top:300px;left:58px;display:flex;gap:16px;font-size:104px;line-height:1.42;font-weight:600}
.cover-page h1 span{writing-mode:vertical-rl;text-orientation:upright}
.cover-page h1 span:last-child{padding-top:130px;font-size:84px}
.tier{font-size:76px;line-height:1.1;font-weight:700;letter-spacing:6px}
.tier-deck{font:21px/1.5 "Noto Sans SC","PingFang SC",sans-serif;color:var(--accent);margin:14px 0 26px;letter-spacing:1px}
.index-page{display:flex;flex-direction:column}
.rows{border-top:3px solid var(--accent);flex:1;display:flex;flex-direction:column;justify-content:space-evenly;margin-bottom:78px}
.row{display:flex;align-items:baseline;gap:26px;border-bottom:1px solid var(--rule-soft);padding:10px 4px}
.num{font:600 40px/1 "Noto Sans SC","PingFang SC",sans-serif;color:var(--accent);min-width:76px;letter-spacing:1px}
.name{font-size:27px;line-height:1.32;font-weight:700;flex:1}
.meta{font:16px/1.4 "Noto Sans SC","PingFang SC",sans-serif;color:var(--accent);display:flex;gap:14px;align-items:baseline;justify-content:flex-end;min-width:220px}
.meta em{font-style:normal;letter-spacing:1px;white-space:nowrap}
.meta b{font-weight:400;letter-spacing:2px;white-space:nowrap}

</style></head><body>
""" + "\n".join(pages) + "\n</body></html>"

HTML = HTML.replace("{{COVER}}", text_uris["COVER"])
out = WORK / "projects-index.html"
out.write_text(HTML, encoding="utf-8")
print("written:", out, out.stat().st_size // 1024, "KB")
