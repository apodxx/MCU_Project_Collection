#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""生成项目 04（自动闸门）的接线示意图。"""

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
RAIL_5V = (230, 81, 0)
RAIL_N = (38, 50, 56)

C_SERVO = (251, 140, 0)
C_ENC_A = (67, 160, 71)
C_ENC_B = (0, 137, 123)
C_TOUCH = (239, 108, 0)
C_IR = (106, 27, 154)
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

    text(d, (W // 2, 28), "项目 04 · 自动闸门 · 接线图", f_title, INK, "ma")
    text(d, (W // 2, 76),
         "STM32F103C8T6 最小系统板　|　红外检测车辆，舵机抬杆", f_sub, MUTED, "ma")

    # ================= 上：信号线 =================
    board = (620, 150, 1000, 830)
    rounded(d, board, 14, BOARD_FILL, BOARD_LINE, 3)
    text(d, (810, 176), "STM32F103C8T6", font(FONT_EN, 24), INK, "ma")
    text(d, (810, 210), "最小系统板（蓝色核心板）", f_mod_sub, MUTED, "ma")

    left_pins = [("PA0", 230), ("PA1", 400), ("PA2", 440), ("PA3", 570), ("PA4", 730)]
    right_pins = [("PB10", 230), ("PB11", 280), ("PB5", 450), ("PA13", 620), ("PA14", 670)]
    for name, y in left_pins:
        text(d, (638, y), name, f_pin, INK, "lm")
        d.ellipse([613, y - 5, 623, y + 5], fill=BOARD_LINE)
    for name, y in right_pins:
        text(d, (982, y), name, f_pin, INK, "rm")
        d.ellipse([997, y - 5, 1007, y + 5], fill=BOARD_LINE)

    text(d, (810, 792), "VCC / GND 接法见下方「面包板电源分配」", f_small, MUTED, "ma")

    module(d, (190, 170, 470, 300), "舵机 SG90", "橙色 = 信号线",
           [("PWM", 230, C_SERVO)], "right", (f_mod, f_mod_sub, f_pin))
    module(d, (190, 330, 470, 470), "旋转编码器", "A/B 正交 · C 接 GND",
           [("A", 400, C_ENC_A), ("B", 440, C_ENC_B)], "right",
           (f_mod, f_mod_sub, f_pin))
    module(d, (190, 500, 470, 630), "触摸模块", "TTP223 单键",
           [("OUT", 570, C_TOUCH)], "right", (f_mod, f_mod_sub, f_pin))
    module(d, (190, 660, 470, 790), "红外避障", "检测到车 → 低电平",
           [("OUT", 730, C_IR)], "right", (f_mod, f_mod_sub, f_pin))

    module(d, (1180, 170, 1460, 340), "OLED 0.96", "SSD1306 · I2C",
           [("SCL", 230, C_SCL), ("SDA", 280, C_SDA)], "left",
           (f_mod, f_mod_sub, f_pin))
    module(d, (1180, 380, 1460, 510), "蜂鸣器", "低电平触发",
           [("IN", 450, C_BUZZ)], "left", (f_mod, f_mod_sub, f_pin))
    module(d, (1180, 540, 1460, 700), "ST-Link", "下载 / 调试",
           [("SWDIO", 620, C_SWD), ("SWCLK", 670, C_SWD)], "left",
           (f_mod, f_mod_sub, f_pin))

    d.line([(470, 230), (613, 230)], fill=C_SERVO, width=3)
    text(d, (542, 220), "PWM", f_legend, C_SERVO, "ms")
    d.line([(470, 400), (613, 400)], fill=C_ENC_A, width=3)
    text(d, (542, 390), "A", f_legend, C_ENC_A, "ms")
    d.line([(470, 440), (613, 440)], fill=C_ENC_B, width=3)
    text(d, (542, 430), "B", f_legend, C_ENC_B, "ms")
    d.line([(470, 570), (613, 570)], fill=C_TOUCH, width=3)
    text(d, (542, 560), "OUT", f_legend, C_TOUCH, "ms")
    d.line([(470, 730), (613, 730)], fill=C_IR, width=3)
    text(d, (542, 720), "OUT", f_legend, C_IR, "ms")

    d.line([(1180, 230), (1007, 230)], fill=C_SCL, width=3)
    text(d, (1094, 220), "SCL", f_legend, C_SCL, "ms")
    d.line([(1180, 280), (1007, 280)], fill=C_SDA, width=3)
    text(d, (1094, 270), "SDA", f_legend, C_SDA, "ms")
    d.line([(1180, 450), (1007, 450)], fill=C_BUZZ, width=3)
    text(d, (1094, 440), "IN", f_legend, C_BUZZ, "ms")
    d.line([(1180, 620), (1007, 620)], fill=C_SWD, width=3)
    text(d, (1094, 610), "SWDIO", f_legend, C_SWD, "ms")
    d.line([(1180, 670), (1007, 670)], fill=C_SWD, width=3)
    text(d, (1094, 660), "SWCLK", f_legend, C_SWD, "ms")

    # ---- 线色图例 ----
    rounded(d, (190, 822, 1000, 906), 8, (248, 249, 250), (222, 226, 230), 1)
    text(d, (204, 838), "线色", f_legend, MUTED, "lm")
    col1 = [(C_SERVO, "舵机信号"), (C_ENC_A, "编码器 A"), (C_ENC_B, "编码器 B"),
            (C_TOUCH, "触摸 OUT")]
    col2 = [(C_IR, "红外 OUT"), (C_BUZZ, "蜂鸣器 IN"), (C_SCL, "I2C SCL"), (C_SDA, "I2C SDA")]
    col3 = [(C_SWD, "SWD 调试")]
    for col, x in ((col1, 250), (col2, 470), (col3, 700)):
        for i, (color, label) in enumerate(col):
            y = 842 + i * 17
            d.line([(x, y), (x + 26, y)], fill=color, width=4)
            text(d, (x + 32, y), label, f_legend, INK, "lm")

    # ================= 分隔 =================
    d.line([(60, 930), (W - 60, 930)], fill=(222, 226, 230), width=2)
    text(d, (60, 948),
         "面包板电源分配：舵机单独 5V，其余模块 3V3，GND 全部共地", f_panel, INK, "la")

    # ================= 下：电源分配 =================
    rail_y = 1040
    gnd_y = 1215
    x0, x1 = 90, 1560

    d.line([(240, rail_y), (1240, rail_y)], fill=RAIL_P, width=9)
    d.line([(1270, rail_y), (1560, rail_y)], fill=RAIL_5V, width=9)
    d.line([(x0, gnd_y), (x1, gnd_y)], fill=RAIL_N, width=9)

    text(d, (240, rail_y - 30), "+ 3V3（面包板 + 轨）", f_mod, RAIL_P, "la")
    text(d, (1270, rail_y - 30), "+ 5V（舵机专用）", f_mod, RAIL_5V, "la")
    text(d, (x0, gnd_y + 14), "- GND（面包板 - 轨）", f_mod, RAIL_N, "la")

    for x in range(x0 + 30, x1 - 10, 34):
        d.ellipse([x - 2, gnd_y - 2, x + 2, gnd_y + 2], fill=(120, 130, 136))

    board2 = (120, 1055, 330, 1180)
    rounded(d, board2, 10, BOARD_FILL, BOARD_LINE, 2)
    text(d, (225, 1076), "STM32 板", f_mod, INK, "ma")
    text(d, (225, 1106), "3V3 → + 轨", f_small, MUTED, "ma")
    text(d, (225, 1128), "GND → - 轨", f_small, MUTED, "ma")

    d.line([(225, board2[1]), (225, rail_y)], fill=RAIL_P, width=4)
    d.line([(225, board2[3]), (225, gnd_y)], fill=RAIL_N, width=4)

    mods = [
        (370, 520, "红外避障", "vcc3"),
        (550, 700, "OLED", "vcc3"),
        (730, 880, "编码器", "gnd"),
        (910, 1060, "触摸模块", "vcc3"),
        (1090, 1240, "蜂鸣器", "vcc3"),
        (1270, 1520, "舵机 SG90", "vcc5"),
    ]
    for mx0, mx1, name, kind in mods:
        box = (mx0, 1055, mx1, 1180)
        rounded(d, box, 10, MOD_FILL, MOD_LINE, 2)
        text(d, ((mx0 + mx1) // 2, 1076), name, f_mod, INK, "ma")
        cx = (mx0 + mx1) // 2
        if kind == "vcc3":
            text(d, (cx, 1106), "VCC → + 轨", f_small, MUTED, "ma")
            d.line([(cx, box[1]), (cx, rail_y)], fill=RAIL_P, width=3)
            d.ellipse([cx - 6, rail_y - 6, cx + 6, rail_y + 6], fill=RAIL_P)
        elif kind == "vcc5":
            text(d, (cx, 1106), "VCC → 5V", f_small, RAIL_5V, "ma")
            d.line([(cx, box[1]), (cx, rail_y)], fill=RAIL_5V, width=3)
            d.ellipse([cx - 6, rail_y - 6, cx + 6, rail_y + 6], fill=RAIL_5V)
        else:
            text(d, (cx, 1106), "C 接 GND", f_small, MUTED, "ma")
        text(d, (cx, 1128), "GND → - 轨", f_small, MUTED, "ma")
        d.line([(cx, box[3]), (cx, gnd_y)], fill=RAIL_N, width=3)
        d.ellipse([cx - 6, gnd_y - 6, cx + 6, gnd_y + 6], fill=RAIL_N)

    text(d, (60, 1252),
         "注意：SG90 堵转电流可达 700mA，别用 ST-Link 的 3.3V 带舵机；USB 供电时舵机接板子 5V，"
         "外接电源要共地", f_note, (198, 40, 40), "la")

    img.save(out)
    print("saved:", out)


if __name__ == "__main__":
    main()
