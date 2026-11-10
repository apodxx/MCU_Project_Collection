#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""生成项目 01 的接线示意图（不依赖第三方库，只用 Pillow）。

用法：python gen_wiring.py [输出路径]
默认输出同目录下的 wiring.png。
"""

import os
import sys

from PIL import Image, ImageDraw, ImageFont

W, H = 1500, 980
BG = (255, 255, 255)
INK = (33, 37, 41)
MUTED = (110, 117, 125)
BOARD_FILL = (241, 248, 233)
BOARD_LINE = (46, 125, 50)
MOD_FILL = (227, 242, 253)
MOD_LINE = (30, 136, 229)
C_SIGNAL = (67, 160, 71)
C_TOUCH = (251, 140, 0)
C_BUZZER = (249, 168, 37)
C_SCL = (30, 136, 229)
C_SDA = (142, 36, 170)
C_SWD = (84, 110, 122)
C_UART = (0, 172, 193)

FONT_CN = r"C:\Windows\Fonts\simhei.ttf"
FONT_EN = r"C:\Windows\Fonts\consolab.ttf"


def font(path, size):
    return ImageFont.truetype(path, size)


def rounded(draw, box, radius, fill, outline, width=2):
    draw.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=width)


def text(draw, xy, s, f, fill=INK, anchor="la"):
    draw.text(xy, s, font=f, fill=fill, anchor=anchor)


def module(draw, box, title, subtitle, pin_specs, side, title_f, sub_f, pin_f):
    """pin_specs: [(label, y, color)] —— 画在模块靠近板子的一侧"""
    rounded(draw, box, 10, MOD_FILL, MOD_LINE, 2)
    x0, y0, x1, y1 = box
    text(draw, ((x0 + x1) // 2, y0 + 14), title, title_f, INK, "ma")
    if subtitle:
        text(draw, ((x0 + x1) // 2, y0 + 42), subtitle, sub_f, MUTED, "ma")
    for label, y, color in pin_specs:
        if side == "right":
            text(draw, (x1 - 10, y), label, pin_f, color, "rm")
            draw.ellipse([x1 - 7, y - 5, x1 + 3, y + 5], fill=color)
        else:
            text(draw, (x0 + 10, y), label, pin_f, color, "lm")
            draw.ellipse([x0 - 3, y - 5, x0 + 7, y + 5], fill=color)


def wire(draw, p1, p2, color, dash=False, width=3):
    if not dash:
        draw.line([p1, p2], fill=color, width=width)
        return
    x1, y1 = p1
    x2, y2 = p2
    step = 14
    x = min(x1, x2)
    while x < max(x1, x2):
        x_end = min(x + step, max(x1, x2))
        draw.line([(x, y1), (x_end, y2)], fill=color, width=width)
        x += step * 2


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "wiring.png")
    img = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(img)

    f_title = font(FONT_CN, 32)
    f_sub = font(FONT_CN, 17)
    f_mod = font(FONT_CN, 20)
    f_mod_sub = font(FONT_CN, 15)
    f_pin = font(FONT_EN, 17)
    f_note = font(FONT_CN, 17)
    f_legend = font(FONT_CN, 16)

    text(d, (W // 2, 34), "项目 01 · 智能温湿度监测系统 · 接线图", f_title, INK, "ma")
    text(d, (W // 2, 82), "STM32F103C8T6 最小系统板　|　按板上丝印找引脚，图中位置仅示意　|　所有模块 VCC 接 3V3、GND 接 GND",
         f_sub, MUTED, "ma")

    # ---------------- 主控板 ----------------
    board = (560, 150, 940, 860)
    rounded(d, board, 14, BOARD_FILL, BOARD_LINE, 3)
    text(d, (750, 176), "STM32F103C8T6", font(FONT_EN, 24), INK, "ma")
    text(d, (750, 210), "最小系统板（蓝色核心板）", f_mod_sub, MUTED, "ma")

    left_pins = [("PA1", 250), ("PA2", 470), ("PB5", 690)]
    right_pins = [("PB10", 250), ("PB11", 300), ("PA13", 520), ("PA14", 570), ("PA9", 700), ("PA10", 750)]
    for name, y in left_pins:
        text(d, (578, y), name, f_pin, INK, "lm")
        d.ellipse([553, y - 5, 563, y + 5], fill=BOARD_LINE)
    for name, y in right_pins:
        text(d, (922, y), name, f_pin, INK, "rm")
        d.ellipse([937, y - 5, 947, y + 5], fill=BOARD_LINE)

    text(d, (750, 800), "3V3  →  面包板红色电源轨", f_mod_sub, MUTED, "ma")
    text(d, (750, 824), "GND →  面包板蓝色电源轨", f_mod_sub, MUTED, "ma")

    # ---------------- 左侧模块 ----------------
    module(d, (120, 190, 400, 320), "DHT11", "温湿度传感器",
           [("DATA", 250, C_SIGNAL)], "right", f_mod, f_mod_sub, f_pin)
    module(d, (120, 410, 400, 540), "触摸模块", "TTP223 单键",
           [("OUT", 470, C_TOUCH)], "right", f_mod, f_mod_sub, f_pin)
    module(d, (120, 630, 400, 760), "有源蜂鸣器", "低电平触发",
           [("IN", 690, C_BUZZER)], "right", f_mod, f_mod_sub, f_pin)

    # ---------------- 右侧模块 ----------------
    module(d, (1100, 190, 1380, 360), "OLED 0.96", "SSD1306 · I2C",
           [("SCL", 250, C_SCL), ("SDA", 300, C_SDA)], "left", f_mod, f_mod_sub, f_pin)
    module(d, (1100, 440, 1380, 620), "ST-Link", "下载 / 调试",
           [("SWDIO", 520, C_SWD), ("SWCLK", 570, C_SWD)], "left", f_mod, f_mod_sub, f_pin)
    module(d, (1100, 650, 1380, 800), "USB-TTL（可选）", "看串口日志",
           [("RX", 700, C_UART), ("TX", 750, C_UART)], "left", f_mod, f_mod_sub, f_pin)

    # ---------------- 信号线 ----------------
    wire(d, (400, 250), (553, 250), C_SIGNAL)
    text(d, (476, 240), "DATA", f_legend, C_SIGNAL, "ms")

    wire(d, (400, 470), (553, 470), C_TOUCH)
    text(d, (476, 460), "OUT", f_legend, C_TOUCH, "ms")

    wire(d, (400, 690), (553, 690), C_BUZZER)
    text(d, (476, 680), "IN", f_legend, C_BUZZER, "ms")

    wire(d, (1100, 250), (947, 250), C_SCL)
    text(d, (1024, 240), "SCL", f_legend, C_SCL, "ms")

    wire(d, (1100, 300), (947, 300), C_SDA)
    text(d, (1024, 290), "SDA", f_legend, C_SDA, "ms")

    wire(d, (1100, 520), (947, 520), C_SWD)
    text(d, (1024, 510), "SWDIO", f_legend, C_SWD, "ms")

    wire(d, (1100, 570), (947, 570), C_SWD)
    text(d, (1024, 560), "SWCLK", f_legend, C_SWD, "ms")

    wire(d, (1100, 700), (947, 700), C_UART, dash=True)
    text(d, (1024, 690), "RX", f_legend, C_UART, "ms")

    wire(d, (1100, 750), (947, 750), C_UART, dash=True)
    text(d, (1024, 740), "TX", f_legend, C_UART, "ms")

    # ---------------- 图例 ----------------
    legend_box = (120, 800, 520, 900)
    rounded(d, legend_box, 8, (248, 249, 250), (222, 226, 230), 1)
    text(d, (136, 812), "线色说明", f_legend, MUTED, "la")
    items = [(C_SIGNAL, "单总线数据"), (C_TOUCH, "触摸输入"), (C_BUZZER, "蜂鸣器")]
    for i, (color, label) in enumerate(items):
        y = 840 + i * 20
        d.line([(140, y), (176, y)], fill=color, width=4)
        text(d, (184, y), label, f_legend, INK, "lm")

    items2 = [(C_SCL, "I2C SCL"), (C_SDA, "I2C SDA"), (C_SWD, "SWD 调试"), (C_UART, "串口（可选）")]
    for i, (color, label) in enumerate(items2):
        y = 840 + i * 20
        x = 330
        if i < 3:
            d.line([(x, y), (x + 36, y)], fill=color, width=4)
        else:
            wire(d, (x, y), (x + 36, y), color, dash=True, width=4)
        text(d, (x + 44, y), label, f_legend, INK, "lm")

    text(d, (560, 910), "注意：只用 ST-Link 供电时，板子的 5V 引脚没有电，所有模块一律接 3V3",
         f_note, (198, 40, 40), "la")

    img.save(out)
    print("saved:", out)


if __name__ == "__main__":
    main()
