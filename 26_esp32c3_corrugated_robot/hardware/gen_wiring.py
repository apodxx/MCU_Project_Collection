#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Generate the wiring diagram for the corrugated robot (Pillow).

Top: signal wires, ESP32-C3 in the middle.
Bottom: power topology -- servos on their own 5V rail, logic on 3V3,
everything sharing GND.

Usage:
    python gen_wiring.py [output.png]
"""

from __future__ import annotations

import os
import sys

from PIL import Image, ImageDraw, ImageFont

W, H = 1800, 1360
BG = (255, 255, 255)
INK = (33, 37, 41)
MUTED = (105, 112, 120)
LINE = (218, 223, 228)
GREEN = (46, 125, 50)
GREEN_FILL = (241, 248, 233)
BLUE = (30, 100, 180)
BLUE_FILL = (232, 242, 253)
ORANGE = (230, 120, 20)
ORANGE_FILL = (255, 246, 232)
RED = (198, 40, 40)
BLACK = (45, 50, 55)
PURPLE = (123, 63, 160)
TEAL = (0, 121, 107)
PINK = (194, 24, 91)

FONT_CN = r"C:\Windows\Fonts\simhei.ttf"
FONT_EN = r"C:\Windows\Fonts\consolab.ttf"


def font(path: str, size: int) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(path, size)


def rounded(d, box, radius, fill, outline, width=2):
    d.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=width)


def text(d, xy, s, f, fill=INK, anchor="la"):
    d.text(xy, s, font=f, fill=fill, anchor=anchor)


def arrow(d, x0, y0, x1, y1, color, width=4):
    d.line([(x0, y0), (x1, y1)], fill=color, width=width)
    # right-pointing arrow head
    d.polygon([(x1, y1), (x1 - 12, y1 - 7), (x1 - 12, y1 + 7)], fill=color)


def module(d, box, title, subtitle, pin_label, pin_y, pin_color, side, fonts):
    f_title, f_sub, f_pin = fonts
    rounded(d, box, 10, BLUE_FILL, BLUE, 2)
    x0, y0, x1, y1 = box
    text(d, ((x0 + x1) // 2, y0 + 14), title, f_title, INK, "ma")
    if subtitle:
        text(d, ((x0 + x1) // 2, y0 + 44), subtitle, f_sub, MUTED, "ma")

    if side == "right":
        text(d, (x1 - 12, pin_y), pin_label, f_pin, pin_color, "rm")
        d.ellipse([x1 - 6, pin_y - 6, x1 + 6, pin_y + 6], fill=pin_color)
    else:
        text(d, (x0 + 12, pin_y), pin_label, f_pin, pin_color, "lm")
        d.ellipse([x0 - 6, pin_y - 6, x0 + 6, pin_y + 6], fill=pin_color)


def pin(d, x, y, label, color, f_pin, side):
    d.ellipse([x - 6, y - 6, x + 6, y + 6], fill=color)
    if side == "left":
        text(d, (x - 12, y), label, f_pin, INK, "rm")
    else:
        text(d, (x + 12, y), label, f_pin, INK, "lm")


def wire(d, x0, y0, x1, y1, color, label, f_label):
    d.line([(x0, y0), (x1, y1)], fill=color, width=4)
    if label:
        text(d, ((x0 + x1) // 2, y0 - 12), label, f_label, color, "ma")


def main() -> None:
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "wiring.png")

    img = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(img)

    f_title = font(FONT_CN, 36)
    f_sub = font(FONT_CN, 18)
    f_section = font(FONT_CN, 23)
    f_mod = font(FONT_CN, 21)
    f_mod_sub = font(FONT_CN, 15)
    f_pin = font(FONT_EN, 17)
    f_label = font(FONT_CN, 14)
    f_note = font(FONT_CN, 17)
    f_small = font(FONT_CN, 14)
    f_tiny = font(FONT_CN, 13)

    # ---------------- header ----------------
    text(d, (W // 2, 28), "瓦楞机器人 · ESP32-C3 · 接线图", f_title, INK, "ma")
    text(d, (W // 2, 78),
         "按板上丝印找引脚，图中位置仅示意　|　信号 3.3V 逻辑　|　舵机单独 5V 供电",
         f_sub, MUTED, "ma")

    # ---------------- board ----------------
    board = (720, 145, 1090, 790)
    rounded(d, board, 16, GREEN_FILL, GREEN, 3)
    text(d, (905, 178), "ESP32-C3", font(FONT_EN, 26), INK, "ma")
    text(d, (905, 214), "SuperMini / DevKitM-1", f_mod_sub, MUTED, "ma")

    # USB-C notch
    rounded(d, (860, 130, 950, 150), 6, (236, 239, 241), (130, 138, 145), 2)
    text(d, (905, 122), "USB-C 供电 / 烧录", f_tiny, MUTED, "md")

    left_pins = [
        (290, "GPIO3", ORANGE),
        (450, "GPIO0", TEAL),
        (620, "GPIO1", PURPLE),
    ]
    right_pins = [
        (260, "GPIO4", BLUE),
        (310, "GPIO5", PINK),
        (480, "GPIO6", RED),
        (660, "GPIO7", RED),
    ]
    for y, label, color in left_pins:
        pin(d, 714, y, label, color, f_pin, "right")
    for y, label, color in right_pins:
        pin(d, 1096, y, label, color, f_pin, "left")

    # board power note
    text(d, (905, 724), "电源引脚从板子边缘引出", f_small, MUTED, "ma")
    pin(d, 790, 762, "3V3", ORANGE, f_pin, "left")
    pin(d, 1020, 762, "GND", BLACK, f_pin, "right")
    text(d, (905, 775), "电源接法见下方", f_tiny, MUTED, "ma")

    # ---------------- left modules ----------------
    module(d, (150, 220, 500, 360), "TTP223 触摸模块",
           "摸一下 → 换表情 + 点头", "OUT", 290, ORANGE, "right",
           (f_mod, f_mod_sub, f_pin))
    module(d, (150, 390, 500, 510), "红外避障 · 左手边",
           "TCRT5000 / 红外对管", "DO", 450, TEAL, "right",
           (f_mod, f_mod_sub, f_pin))
    module(d, (150, 560, 500, 680), "红外避障 · 右手边",
           "手靠近 → 左右摇头", "DO", 620, PURPLE, "right",
           (f_mod, f_mod_sub, f_pin))

    # ---------------- right modules ----------------
    module(d, (1300, 190, 1650, 370), "0.96\" OLED",
           "SSD1306 · 128×64 · I2C", "SDA", 260, BLUE, "left",
           (f_mod, f_mod_sub, f_pin))
    text(d, (1312, 310), "SCL", f_pin, PINK, "lm")
    d.ellipse([1294, 304, 1306, 316], fill=PINK)

    module(d, (1300, 410, 1650, 540), "水平舵机 · 左右摇头",
           "SG90 / MG90S · 信号线", "SIG", 480, RED, "left",
           (f_mod, f_mod_sub, f_pin))
    module(d, (1300, 590, 1650, 720), "垂直舵机 · 上下点头",
           "SG90 / MG90S · 信号线", "SIG", 660, RED, "left",
           (f_mod, f_mod_sub, f_pin))

    # ---------------- signal wires ----------------
    wire(d, 500, 290, 714, 290, ORANGE, "GPIO3", f_label)
    wire(d, 500, 450, 714, 450, TEAL, "GPIO0", f_label)
    wire(d, 500, 620, 714, 620, PURPLE, "GPIO1", f_label)
    wire(d, 1096, 260, 1300, 260, BLUE, "GPIO4", f_label)
    wire(d, 1096, 310, 1300, 310, PINK, "GPIO5", f_label)
    wire(d, 1096, 480, 1300, 480, RED, "GPIO6", f_label)
    wire(d, 1096, 660, 1300, 660, RED, "GPIO7", f_label)

    # ---------------- callouts ----------------
    rounded(d, (150, 735, 620, 805), 8, ORANGE_FILL, (245, 190, 130), 1)
    text(d, (168, 752), "红外模块 VCC 接 3V3：它的 DO 输出才是 3.3V，", f_small, INK)
    text(d, (168, 776), "ESP32-C3 引脚不耐 5V。", f_small, INK)

    rounded(d, (1180, 748, 1650, 812), 8, (255, 235, 238), (240, 160, 170), 1)
    text(d, (1198, 766), "舵机红线（V+）不要接 3V3；转一下就重启/花屏，", f_small, RED)
    text(d, (1198, 788), "基本都是供电电流不够。", f_small, RED)

    # ---------------- divider ----------------
    d.line([(60, 838), (W - 60, 838)], fill=LINE, width=2)
    text(d, (60, 858), "供电拓扑", f_section, INK, "la")
    text(d, (200, 862),
         "舵机独立 5V；OLED / 触摸 / 红外用 3V3；所有 GND 必须连在一起",
         f_note, MUTED, "la")
    text(d, (W - 60, 862),
         "舵机 5V / GND 之间并 470~1000uF 电解电容（靠近舵机）",
         f_small, RED, "ra")

    # ---------------- power rows ----------------
    row_specs = [
        (990, "5V 2A 电源", "充电宝 / USB 适配器", RED, "5V",
         ["水平舵机 V+", "垂直舵机 V+"]),
        (1125, "ESP32-C3 3V3", "板载稳压输出", ORANGE, "3.3V",
         ["OLED VCC", "TTP223 VCC", "红外 × 2 VCC"]),
        (1255, "公共 GND", "所有负极连在一起", BLACK, "GND",
         ["ESP32 GND", "两个舵机 GND", "所有模块 GND"]),
    ]

    for y, src_title, src_sub, color, bus_label, loads in row_specs:
        src_box = (70, y - 46, 320, y + 46)
        rounded(d, src_box, 10, (248, 249, 250), color, 2)
        text(d, (195, y - 22), src_title, f_mod, INK, "ma")
        text(d, (195, y + 14), src_sub, f_tiny, MUTED, "ma")

        bus_x0, bus_x1 = 350, 1680
        d.line([(bus_x0, y), (bus_x1, y)], fill=color, width=6)
        text(d, (bus_x0 + 8, y - 15), bus_label, f_pin, color, "lm")

        # Draw the loads above the rail, each with its own tap.  This is a
        # parallel bus, not a daisy chain.
        if len(loads) == 2:
            boxes = [(540, 300), (980, 300)]
        else:
            boxes = [(500, 240), (790, 240), (1080, 300)]

        for (x, width), load in zip(boxes, loads):
            box = (x, y - 108, x + width, y - 26)
            fill = (255, 248, 248) if color == RED else (
                (255, 249, 238) if color == ORANGE else (247, 248, 249))
            rounded(d, box, 8, fill, color, 2)
            text(d, (x + width // 2, y - 67), load, f_tiny, INK, "ma")
            tap = x + width // 2
            d.line([(tap, y - 26), (tap, y)], fill=color, width=3)
            d.ellipse([tap - 6, y - 6, tap + 6, y + 6], fill=color)

    # ---------------- notes ----------------
    d.line([(60, 1305), (W - 60, 1305)], fill=LINE, width=1)
    text(d, (60, 1316),
         "上电检查：先只接 USB 看屏幕 → 再单独给舵机上 5V → 共地 → 摸触摸/红外测试",
         f_note, INK, "la")
    text(d, (W - 60, 1316),
         "舵机电流峰值可达 1A+，两个舵机不要从 3V3 取电", f_note, RED, "ra")

    img.save(out)
    print("saved:", out)


if __name__ == "__main__":
    main()
