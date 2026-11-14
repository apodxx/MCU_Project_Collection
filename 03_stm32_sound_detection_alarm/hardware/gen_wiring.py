#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""生成项目 03 的接线示意图（Pillow 绘制）。

上：信号线接法
下：面包板电源分配（3V3→+ 轨，GND→- 轨，模块 VCC→+、GND→-）

用法：python gen_wiring.py [输出路径]
"""

import os
import sys

from PIL import Image, ImageDraw, ImageFont

W, H = 1650, 1300
BG = (255, 255, 255)
INK = (33, 37, 41)
MUTED = (110, 117, 125)
BOARD_FILL = (241, 248, 233)
BOARD_LINE = (46, 125, 50)
MOD_FILL = (227, 242, 253)
MOD_LINE = (30, 136, 229)
RAIL_P = (211, 47, 47)
RAIL_N = (38, 50, 56)

C_SOUND = (251, 140, 0)
C_ENC_A = (67, 160, 71)
C_ENC_B = (0, 137, 123)
C_SCL = (30, 136, 229)
C_SDA = (142, 36, 170)
C_BUZZ = (249, 168, 37)
C_SWD = (84, 110, 122)

FONT_CN = r"C:\Windows\Fonts\simhei.ttf"
FONT_EN = r"C:\Windows\Fonts\consolab.ttf"


def font(path, size):
    return ImageFont.truetype(path, size)


def rounded(draw, box, radius, fill, outline, width=2):
    draw.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=width)


def text(draw, xy, s, f, fill=INK, anchor="la"):
    draw.text(xy, s, font=f, fill=fill, anchor=anchor)


def module(draw, box, title, subtitle, pins, side, fonts):
    f_mod, f_sub, f_pin = fonts
    rounded(draw, box, 10, MOD_FILL, MOD_LINE, 2)
    x0, y0, x1, y1 = box
    text(draw, ((x0 + x1) // 2, y0 + 12), title, f_mod, INK, "ma")
    if subtitle:
        text(draw, ((x0 + x1) // 2, y0 + 40), subtitle, f_sub, MUTED, "ma")
    for label, y, color in pins:
        if side == "right":
            text(draw, (x1 - 12, y), label, f_pin, color, "rm")
            draw.ellipse([x1 - 6, y - 5, x1 + 4, y + 5], fill=color)
        else:
            text(draw, (x0 + 12, y), label, f_pin, color, "lm")
            draw.ellipse([x0 - 4, y - 5, x0 + 6, y + 5], fill=color)


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "wiring.png")

    img = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(img)

    f_title = font(FONT_CN, 32)
    f_sub = font(FONT_CN, 17)
    f_mod = font(FONT_CN, 19)
    f_mod_sub = font(FONT_CN, 14)
    f_pin = font(FONT_EN, 16)
    f_note = font(FONT_CN, 17)
    f_legend = font(FONT_CN, 14)
    f_panel = font(FONT_CN, 19)
    f_small = font(FONT_CN, 13)

    text(d, (W // 2, 28), "项目 03 · 智能声音检测与报警 · 接线图", f_title, INK, "ma")
    text(d, (W // 2, 76),
         "STM32F103C8T6 最小系统板　|　按板上丝印找引脚，图中位置仅示意", f_sub, MUTED, "ma")

    # ================= 上：信号线 =================
    board = (620, 150, 1000, 768)
    rounded(d, board, 14, BOARD_FILL, BOARD_LINE, 3)
    text(d, (810, 176), "STM32F103C8T6", font(FONT_EN, 24), INK, "ma")
    text(d, (810, 210), "最小系统板（蓝色核心板）", f_mod_sub, MUTED, "ma")

    left_pins = [("PA0", 250), ("PA1", 470), ("PA2", 510)]
    right_pins = [("PB10", 250), ("PB11", 300), ("PB5", 520), ("PA13", 620), ("PA14", 670)]
    for name, y in left_pins:
        text(d, (638, y), name, f_pin, INK, "lm")
        d.ellipse([613, y - 5, 623, y + 5], fill=BOARD_LINE)
    for name, y in right_pins:
        text(d, (982, y), name, f_pin, INK, "rm")
        d.ellipse([997, y - 5, 1007, y + 5], fill=BOARD_LINE)

    text(d, (810, 730), "VCC / GND 接法见下方「面包板电源分配」", f_small, MUTED, "ma")

    module(d, (190, 190, 470, 320), "声音传感器", "AO 模拟量",
           [("AO", 250, C_SOUND)], "right", (f_mod, f_mod_sub, f_pin))
    module(d, (190, 420, 470, 560), "旋转编码器", "A/B 正交 · C 接 GND",
           [("A", 470, C_ENC_A), ("B", 510, C_ENC_B)], "right",
           (f_mod, f_mod_sub, f_pin))

    module(d, (1180, 190, 1460, 360), "OLED 0.96", "SSD1306 · I2C",
           [("SCL", 250, C_SCL), ("SDA", 300, C_SDA)], "left",
           (f_mod, f_mod_sub, f_pin))
    module(d, (1180, 460, 1460, 590), "蜂鸣器", "低电平触发",
           [("IN", 520, C_BUZZ)], "left", (f_mod, f_mod_sub, f_pin))
    module(d, (1180, 600, 1460, 740), "ST-Link", "下载 / 调试",
           [("SWDIO", 620, C_SWD), ("SWCLK", 670, C_SWD)], "left",
           (f_mod, f_mod_sub, f_pin))

    d.line([(470, 250), (613, 250)], fill=C_SOUND, width=3)
    text(d, (542, 240), "AO", f_legend, C_SOUND, "ms")
    d.line([(470, 470), (613, 470)], fill=C_ENC_A, width=3)
    text(d, (542, 460), "A", f_legend, C_ENC_A, "ms")
    d.line([(470, 510), (613, 510)], fill=C_ENC_B, width=3)
    text(d, (542, 500), "B", f_legend, C_ENC_B, "ms")

    d.line([(1180, 250), (1007, 250)], fill=C_SCL, width=3)
    text(d, (1094, 240), "SCL", f_legend, C_SCL, "ms")
    d.line([(1180, 300), (1007, 300)], fill=C_SDA, width=3)
    text(d, (1094, 290), "SDA", f_legend, C_SDA, "ms")
    d.line([(1180, 520), (1007, 520)], fill=C_BUZZ, width=3)
    text(d, (1094, 510), "IN", f_legend, C_BUZZ, "ms")
    d.line([(1180, 620), (1007, 620)], fill=C_SWD, width=3)
    text(d, (1094, 610), "SWDIO", f_legend, C_SWD, "ms")
    d.line([(1180, 670), (1007, 670)], fill=C_SWD, width=3)
    text(d, (1094, 660), "SWCLK", f_legend, C_SWD, "ms")

    # ---- 线色图例 ----
    rounded(d, (190, 776, 1000, 846), 8, (248, 249, 250), (222, 226, 230), 1)
    text(d, (206, 792), "线色", f_legend, MUTED, "lm")
    col1 = [(C_SOUND, "声音 AO"), (C_ENC_A, "编码器 A"), (C_ENC_B, "编码器 B")]
    col2 = [(C_SCL, "I2C SCL"), (C_SDA, "I2C SDA"), (C_BUZZ, "蜂鸣器 IN"), (C_SWD, "SWD 调试")]
    for i, (color, label) in enumerate(col1):
        y = 792 + i * 18
        d.line([(258, y), (288, y)], fill=color, width=4)
        text(d, (296, y), label, f_legend, INK, "lm")
    for i, (color, label) in enumerate(col2):
        y = 792 + i * 18
        d.line([(510, y), (540, y)], fill=color, width=4)
        text(d, (548, y), label, f_legend, INK, "lm")

    # ================= 分隔 =================
    d.line([(60, 872), (W - 60, 872)], fill=(222, 226, 230), width=2)
    text(d, (60, 890),
         "面包板电源分配：3V3 → + 轨，GND → - 轨；模块 VCC → +、GND → -", f_panel, INK, "la")

    # ================= 下：电源分配 =================
    rail_p_y = 970
    rail_n_y = 1210
    x0, x1 = 90, 1560

    d.line([(x0, rail_p_y), (x1, rail_p_y)], fill=RAIL_P, width=9)
    d.line([(x0, rail_n_y), (x1, rail_n_y)], fill=RAIL_N, width=9)
    text(d, (x0, rail_p_y - 26), "+ 3V3（面包板 + 轨）", f_mod, RAIL_P, "la")
    text(d, (x0, rail_n_y + 12), "- GND（面包板 - 轨）", f_mod, RAIL_N, "la")

    for x in range(x0 + 30, x1 - 10, 34):
        d.ellipse([x - 2, rail_p_y - 2, x + 2, rail_p_y + 2], fill=(255, 205, 210))
        d.ellipse([x - 2, rail_n_y - 2, x + 2, rail_n_y + 2], fill=(120, 130, 136))

    board2 = (140, 1030, 380, 1160)
    rounded(d, board2, 10, BOARD_FILL, BOARD_LINE, 2)
    text(d, (260, 1052), "STM32 板", f_mod, INK, "ma")
    text(d, (260, 1082), "3V3 → + 轨", f_small, MUTED, "ma")
    text(d, (260, 1104), "GND → - 轨", f_small, MUTED, "ma")

    bx = 260
    d.line([(bx, board2[1]), (bx, rail_p_y)], fill=RAIL_P, width=4)
    d.ellipse([bx - 6, rail_p_y - 6, bx + 6, rail_p_y + 6], fill=RAIL_P)
    d.line([(bx, board2[3]), (bx, rail_n_y)], fill=RAIL_N, width=4)
    d.ellipse([bx - 6, rail_n_y - 6, bx + 6, rail_n_y + 6], fill=RAIL_N)

    mods = [
        (440, 660, "声音传感器", True, ""),
        (720, 940, "OLED", True, ""),
        (1000, 1220, "编码器", False, "C 接 GND"),
        (1280, 1500, "蜂鸣器", True, ""),
    ]
    for mx0, mx1, name, has_vcc, note in mods:
        box = (mx0, 1030, mx1, 1160)
        rounded(d, box, 10, MOD_FILL, MOD_LINE, 2)
        text(d, ((mx0 + mx1) // 2, 1052), name, f_mod, INK, "ma")
        cx = (mx0 + mx1) // 2
        if has_vcc:
            text(d, (cx, 1082), "VCC → + 轨", f_small, MUTED, "ma")
            d.line([(cx, box[1]), (cx, rail_p_y)], fill=RAIL_P, width=3)
            d.ellipse([cx - 6, rail_p_y - 6, cx + 6, rail_p_y + 6], fill=RAIL_P)
        else:
            text(d, (cx, 1082), note, f_small, MUTED, "ma")
        text(d, (cx, 1104), "GND → - 轨", f_small, MUTED, "ma")
        d.line([(cx, box[3]), (cx, rail_n_y)], fill=RAIL_N, width=3)
        d.ellipse([cx - 6, rail_n_y - 6, cx + 6, rail_n_y + 6], fill=RAIL_N)

    text(d, (60, 1252),
         "注意：只用 ST-Link 供电时板子的 5V 引脚没有电，模块一律接 3V3（+ 轨）", f_note,
         (198, 40, 40), "la")

    img.save(out)
    print("saved:", out)


if __name__ == "__main__":
    main()
