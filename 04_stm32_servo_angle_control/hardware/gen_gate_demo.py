#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""生成闸门效果示意图：0° 关闸 / 90° 开闸 / 往复。"""

import os
import sys

from PIL import Image, ImageDraw, ImageFont

W, H = 1600, 900
BG = (250, 251, 252)
INK = (33, 37, 41)
MUTED = (110, 117, 125)
BASE = (207, 216, 220)
BASE_LINE = (84, 110, 122)
ARM = (55, 71, 79)
BOARD = (30, 136, 229)
ACCENT = (230, 81, 0)

FONT_CN = r"C:\Windows\Fonts\simhei.ttf"
FONT_EN = r"C:\Windows\Fonts\consolab.ttf"


def f(size, en=False):
    return ImageFont.truetype(FONT_EN if en else FONT_CN, size)


def tx(d, xy, s, font, fill=INK, anchor="la"):
    d.text(xy, s, font=font, fill=fill, anchor=anchor)


def servo(d, cx, ground_y):
    """画舵机底座，返回转轴坐标"""
    d.rounded_rectangle([cx - 80, ground_y - 90, cx + 80, ground_y], radius=10,
                        fill=BASE, outline=BASE_LINE, width=3)
    tx(d, (cx, ground_y - 30), "SG90", f(20, True), INK, "ma")
    d.ellipse([cx - 34, ground_y - 120, cx + 34, ground_y - 56], fill=(236, 239, 241),
              outline=BASE_LINE, width=3)
    d.ellipse([cx - 10, ground_y - 96, cx + 10, ground_y - 76], fill=BASE_LINE)
    return cx, ground_y - 88


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "gate-demo.png")

    img = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(img)

    tx(d, (W // 2, 30), "项目 04 · 自动闸门：实物是什么效果", f(34), INK, "ma")
    tx(d, (W // 2, 78), "舵机转轴带动一根纸板栏杆，0° 水平 = 关闸，90° 竖起 = 开闸", f(18), MUTED, "ma")

    panels = [
        (60, "0°", "关闸（上电默认）", "栏杆水平，挡住通道"),
        (820, "90°", "开闸", "栏杆竖起，车/人可以通过"),
    ]

    for x0, angle, title, desc in panels:
        d.rounded_rectangle([x0, 130, x0 + 720, 760], radius=16, fill=(255, 255, 255),
                            outline=(222, 226, 230), width=2)
        tx(d, (x0 + 24, 150), f"{angle} · {title}", f(24), INK, "la")
        tx(d, (x0 + 24, 186), desc, f(16), MUTED, "la")

        ground = 700
        d.line([(x0 + 40, ground), (x0 + 680, ground)], fill=(176, 190, 197), width=6)

        cx = x0 + 220
        sx, sy = servo(d, cx, ground)

        if angle == "0°":
            # 水平栏杆
            d.rectangle([sx - 14, sy - 16, sx + 360, sy + 16], fill=ARM)
            d.rectangle([sx + 300, sy - 30, sx + 360, sy + 30], fill=ACCENT)
            tx(d, (sx + 170, sy - 52), "纸板栏杆", f(17), INK, "ma")
            # 旋转方向
            d.arc([sx - 130, sy - 130, sx + 130, sy + 130], start=270, end=360,
                  fill=ACCENT, width=5)
            d.polygon([(sx + 128, sy - 6), (sx + 108, sy - 24), (sx + 118, sy + 12)],
                      fill=ACCENT)
            tx(d, (sx + 120, sy + 40), "抬杆", f(16), ACCENT, "lm")
        else:
            # 竖直栏杆
            d.rectangle([sx - 16, sy - 360, sx + 16, sy], fill=ARM)
            d.rectangle([sx - 30, sy - 360, sx + 30, sy - 300], fill=ACCENT)
            tx(d, (sx + 44, sy - 340), "纸板栏杆", f(17), INK, "lm")
            d.arc([sx - 130, sy - 130, sx + 130, sy + 130], start=180, end=270,
                  fill=ACCENT, width=5)
            d.polygon([(sx - 128, sy - 6), (sx - 108, sy - 24), (sx - 118, sy + 12)],
                      fill=ACCENT)
            # 通道箭头
            tx(d, (x0 + 470, ground - 60), "→ 通行", f(22), (46, 125, 50), "lm")

    # 中间循环箭头
    d.arc([700, 380, 900, 580], start=200, end=340, fill=ACCENT, width=6)
    d.polygon([(878, 428), (896, 452), (860, 450)], fill=ACCENT)
    tx(d, (800, 590), "往复模式", f(16), ACCENT, "ma")

    # 底部说明
    d.rounded_rectangle([60, 790, 1540, 870], radius=12, fill=(255, 248, 225),
                        outline=(255, 193, 7), width=2)
    tx(d, (84, 810),
       "屏幕同步显示：模式 手动 / 预设3 / 往复　·　目标 90°　·　当前 88°　·　状态 转动中 → 到位",
       f(17), (95, 70, 0), "la")
    tx(d, (84, 840),
       "操作：转编码器 = 切换模式；手动模式下按触摸 = 抬杆 15°；预设模式下按触摸 = 换预置位；长按 = 关闸",
       f(17), (95, 70, 0), "la")

    img.save(out)
    print("saved:", out)


if __name__ == "__main__":
    main()
