#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""生成闸门效果示意图：关闸/开闸 + 红外检测 + 两种模式。"""

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
ACCENT = (230, 81, 0)
IR_C = (106, 27, 154)
CAR = (30, 136, 229)

FONT_CN = r"C:\Windows\Fonts\simhei.ttf"
FONT_EN = r"C:\Windows\Fonts\consolab.ttf"


def f(size, en=False):
    return ImageFont.truetype(FONT_EN if en else FONT_CN, size)


def tx(d, xy, s, font, fill=INK, anchor="la"):
    d.text(xy, s, font=font, fill=fill, anchor=anchor)


def servo(d, cx, ground_y):
    d.rounded_rectangle([cx - 80, ground_y - 90, cx + 80, ground_y], radius=10,
                        fill=BASE, outline=BASE_LINE, width=3)
    tx(d, (cx, ground_y - 30), "SG90", f(20, True), INK, "ma")
    d.ellipse([cx - 34, ground_y - 120, cx + 34, ground_y - 56], fill=(236, 239, 241),
              outline=BASE_LINE, width=3)
    d.ellipse([cx - 10, ground_y - 96, cx + 10, ground_y - 76], fill=BASE_LINE)
    return cx, ground_y - 88


def car(d, x, ground_y):
    """小车：车身 + 两个轮子"""
    d.rounded_rectangle([x, ground_y - 70, x + 150, ground_y - 18], radius=14,
                        fill=CAR, outline=(13, 71, 161), width=3)
    d.rounded_rectangle([x + 30, ground_y - 100, x + 110, ground_y - 60], radius=10,
                        fill=(144, 202, 249), outline=(13, 71, 161), width=3)
    for wx in (x + 38, x + 112):
        d.ellipse([wx - 18, ground_y - 26, wx + 18, ground_y + 10], fill=(55, 71, 79))
        d.ellipse([wx - 7, ground_y - 15, wx + 7, ground_y - 1], fill=(176, 190, 197))
    tx(d, (x + 75, ground_y - 130), "车", f(18), CAR, "ma")


def ir_module(d, x, ground_y):
    d.rounded_rectangle([x, ground_y - 56, x + 70, ground_y - 10], radius=6,
                        fill=(243, 229, 245), outline=IR_C, width=2)
    d.ellipse([x + 12, ground_y - 46, x + 26, ground_y - 32], fill=IR_C)
    d.ellipse([x + 44, ground_y - 46, x + 58, ground_y - 32], fill=IR_C)
    tx(d, (x + 35, ground_y + 4), "红外", f(15), IR_C, "ma")


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "gate-demo.png")

    img = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(img)

    tx(d, (W // 2, 30), "项目 04 · 自动闸门：红外检测 + 舵机抬杆", f(34), INK, "ma")
    tx(d, (W // 2, 78), "车压到红外 → 自动模式直接抬杆；手动模式先蜂鸣提醒，按触摸才抬杆", f(18),
       MUTED, "ma")

    panels = [
        (60, "关闸（0°）", "车靠近，红外检测到"),
        (820, "开闸（90°）", "栏杆竖起，车通过"),
    ]

    for x0, title, desc in panels:
        d.rounded_rectangle([x0, 130, x0 + 720, 760], radius=16, fill=(255, 255, 255),
                            outline=(222, 226, 230), width=2)
        tx(d, (x0 + 24, 150), title, f(24), INK, "la")
        tx(d, (x0 + 24, 186), desc, f(16), MUTED, "la")

        ground = 700
        d.line([(x0 + 40, ground), (x0 + 680, ground)], fill=(176, 190, 197), width=6)

        cx = x0 + 200
        sx, sy = servo(d, cx, ground)

        car(d, x0 + 520, ground)
        ir_module(d, x0 + 400, ground)

        # 红外虚线
        for gx in range(x0 + 470, x0 + 560, 12):
            d.line([(gx, ground - 34), (gx + 6, ground - 34)], fill=IR_C, width=2)

        if x0 == 60:
            d.rectangle([sx - 14, sy - 16, sx + 290, sy + 16], fill=ARM)
            d.rectangle([sx + 240, sy - 30, sx + 290, sy + 30], fill=ACCENT)
            tx(d, (sx + 130, sy - 52), "纸板栏杆（挡住）", f(16), INK, "ma")
            d.arc([sx - 120, sy - 120, sx + 120, sy + 120], start=270, end=360,
                  fill=ACCENT, width=5)
            d.polygon([(sx + 118, sy - 6), (sx + 98, sy - 24), (sx + 108, sy + 12)], fill=ACCENT)
            tx(d, (x0 + 520, ground - 200), "手动：蜂鸣提醒 → 按触摸", f(16), ACCENT, "ma")
        else:
            d.rectangle([sx - 16, sy - 340, sx + 16, sy], fill=ARM)
            d.rectangle([sx - 30, sy - 340, sx + 30, sy - 285], fill=ACCENT)
            tx(d, (sx + 44, sy - 320), "纸板栏杆（抬起）", f(16), INK, "lm")
            d.arc([sx - 120, sy - 120, sx + 120, sy + 120], start=180, end=270,
                  fill=ACCENT, width=5)
            d.polygon([(sx - 118, sy - 6), (sx - 98, sy - 24), (sx - 108, sy + 12)], fill=ACCENT)
            tx(d, (x0 + 520, ground - 200), "自动：检测到车直接抬杆", f(16), ACCENT, "ma")

    # 底部说明
    d.rounded_rectangle([60, 790, 1540, 870], radius=12, fill=(255, 248, 225),
                        outline=(255, 193, 7), width=2)
    tx(d, (84, 806),
       "自动模式：红外有车 → 开闸；车走后 3 秒 → 自动关闸",
       f(17), (95, 70, 0), "la")
    tx(d, (84, 836),
       "手动模式：红外有车 → 蜂鸣提醒（状态闪「等待」）→ 按触摸开闸/关闸；长按立即关闸",
       f(17), (95, 70, 0), "la")

    img.save(out)
    print("saved:", out)


if __name__ == "__main__":
    main()
