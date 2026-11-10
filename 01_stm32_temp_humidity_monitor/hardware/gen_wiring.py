#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""生成项目 01 的接线示意图（Pillow 绘制）。

上：信号线接法
下：面包板电源分配（3V3→+ 轨，GND→- 轨，所有模块 VCC→+、GND→-）

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


def wire(draw, p1, p2, color, dash=False, width=3):
    if not dash:
        draw.line([p1, p2], fill=color, width=width)
        return
    (x1, y1), (x2, y2) = p1, p2
    step = 14
    x = min(x1, x2)
    while x < max(x1, x2):
        x_end = min(x + step, max(x1, x2))
        draw.line([(x, y1), (x_end, y2)], fill=color, width=width)
        x += step * 2


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "wiring.png")

    img = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(img)

    f_title = font(FONT_CN, 32)
    f_sub = font(FONT_CN, 17)
    f_mod = font(FONT_CN, 20)
    f_mod_sub = font(FONT_CN, 15)
    f_pin = font(FONT_EN, 17)
    f_note = font(FONT_CN, 17)
    f_legend = font(FONT_CN, 15)
    f_panel = font(FONT_CN, 19)
    f_small = font(FONT_CN, 14)

    # ================= 标题 =================
    text(d, (W // 2, 28), "项目 01 · 智能温湿度监测系统 · 接线图", f_title, INK, "ma")
    text(d, (W // 2, 76),
         "STM32F103C8T6 最小系统板　|　按板上丝印找引脚，图中位置仅示意", f_sub, MUTED, "ma")

    # ================= 上：信号线 =================
    board = (620, 150, 1000, 820)
    rounded(d, board, 14, BOARD_FILL, BOARD_LINE, 3)
    text(d, (810, 176), "STM32F103C8T6", font(FONT_EN, 24), INK, "ma")
    text(d, (810, 210), "最小系统板（蓝色核心板）", f_mod_sub, MUTED, "ma")

    left_pins = [("PA1", 250), ("PA2", 470), ("PB5", 690)]
    right_pins = [("PB10", 250), ("PB11", 300), ("PA13", 520),
                  ("PA14", 570), ("PA9", 710), ("PA10", 760)]
    for name, y in left_pins:
        text(d, (638, y), name, f_pin, INK, "lm")
        d.ellipse([613, y - 5, 623, y + 5], fill=BOARD_LINE)
    for name, y in right_pins:
        text(d, (982, y), name, f_pin, INK, "rm")
        d.ellipse([997, y - 5, 1007, y + 5], fill=BOARD_LINE)

    text(d, (810, 782), "VCC / GND 接法见下方「面包板电源分配」", f_small, MUTED, "ma")

    module(d, (190, 190, 470, 320), "DHT11", "温湿度传感器",
           [("DATA", 250, C_SIGNAL)], "right", (f_mod, f_mod_sub, f_pin))
    module(d, (190, 410, 470, 540), "触摸模块", "TTP223 单键",
           [("OUT", 470, C_TOUCH)], "right", (f_mod, f_mod_sub, f_pin))
    module(d, (190, 630, 470, 760), "有源蜂鸣器", "低电平触发",
           [("IN", 690, C_BUZZER)], "right", (f_mod, f_mod_sub, f_pin))

    module(d, (1180, 190, 1460, 360), "OLED 0.96", "SSD1306 · I2C",
           [("SCL", 250, C_SCL), ("SDA", 300, C_SDA)], "left",
           (f_mod, f_mod_sub, f_pin))
    module(d, (1180, 440, 1460, 620), "ST-Link", "下载 / 调试",
           [("SWDIO", 520, C_SWD), ("SWCLK", 570, C_SWD)], "left",
           (f_mod, f_mod_sub, f_pin))
    module(d, (1180, 650, 1460, 800), "USB-TTL（可选）", "看串口日志",
           [("RX", 710, C_UART), ("TX", 760, C_UART)], "left",
           (f_mod, f_mod_sub, f_pin))

    wire(d, (470, 250), (613, 250), C_SIGNAL)
    text(d, (542, 240), "DATA", f_legend, C_SIGNAL, "ms")
    wire(d, (470, 470), (613, 470), C_TOUCH)
    text(d, (542, 460), "OUT", f_legend, C_TOUCH, "ms")
    wire(d, (470, 690), (613, 690), C_BUZZER)
    text(d, (542, 680), "IN", f_legend, C_BUZZER, "ms")

    wire(d, (1180, 250), (1007, 250), C_SCL)
    text(d, (1094, 240), "SCL", f_legend, C_SCL, "ms")
    wire(d, (1180, 300), (1007, 300), C_SDA)
    text(d, (1094, 290), "SDA", f_legend, C_SDA, "ms")
    wire(d, (1180, 520), (1007, 520), C_SWD)
    text(d, (1094, 510), "SWDIO", f_legend, C_SWD, "ms")
    wire(d, (1180, 570), (1007, 570), C_SWD)
    text(d, (1094, 560), "SWCLK", f_legend, C_SWD, "ms")
    wire(d, (1180, 710), (1007, 710), C_UART, dash=True)
    text(d, (1094, 700), "RX", f_legend, C_UART, "ms")
    wire(d, (1180, 760), (1007, 760), C_UART, dash=True)
    text(d, (1094, 750), "TX", f_legend, C_UART, "ms")

    # ================= 分隔 =================
    d.line([(60, 872), (W - 60, 872)], fill=(222, 226, 230), width=2)
    text(d, (60, 890),
         "面包板电源分配：3V3 → + 轨，GND → - 轨；所有模块 VCC → +、GND → -", f_panel, INK, "la")

    # ================= 下：电源分配 =================
    rail_p_y = 970
    rail_n_y = 1210
    x0, x1 = 90, 1560

    d.line([(x0, rail_p_y), (x1, rail_p_y)], fill=RAIL_P, width=9)
    d.line([(x0, rail_n_y), (x1, rail_n_y)], fill=RAIL_N, width=9)
    text(d, (x0, rail_p_y - 26), "+ 3V3（面包板 + 轨）", f_mod, RAIL_P, "la")
    text(d, (x0, rail_n_y + 12), "- GND（面包板 - 轨）", f_mod, RAIL_N, "la")

    # 面包板插孔示意
    for x in range(x0 + 30, x1 - 10, 34):
        d.ellipse([x - 2, rail_p_y - 2, x + 2, rail_p_y + 2], fill=(255, 205, 210))
        d.ellipse([x - 2, rail_n_y - 2, x + 2, rail_n_y + 2], fill=(120, 130, 136))

    # 主控板
    board2 = (140, 1030, 380, 1160)
    rounded(d, board2, 10, BOARD_FILL, BOARD_LINE, 2)
    text(d, (260, 1052), "STM32 板", f_mod, INK, "ma")
    text(d, (260, 1082), "3V3 → + 轨", f_small, MUTED, "ma")
    text(d, (260, 1104), "GND → - 轨", f_small, MUTED, "ma")

    def power_pair(x, top_y, bottom_y):
        """模块顶部接 + 轨，底部接 - 轨"""
        d.line([(x, top_y), (x, rail_p_y)], fill=RAIL_P, width=3)
        d.ellipse([x - 6, rail_p_y - 6, x + 6, rail_p_y + 6], fill=RAIL_P)
        d.line([(x, bottom_y), (x, rail_n_y)], fill=RAIL_N, width=3)
        d.ellipse([x - 6, rail_n_y - 6, x + 6, rail_n_y + 6], fill=RAIL_N)

    # 主控板：3V3 接 + 轨、GND 接 - 轨（各一根）
    bx = 260
    d.line([(bx, board2[1]), (bx, rail_p_y)], fill=RAIL_P, width=4)
    d.ellipse([bx - 6, rail_p_y - 6, bx + 6, rail_p_y + 6], fill=RAIL_P)
    d.line([(bx, board2[3]), (bx, rail_n_y)], fill=RAIL_N, width=4)
    d.ellipse([bx - 6, rail_n_y - 6, bx + 6, rail_n_y + 6], fill=RAIL_N)

    mods = [
        (450, 610, "DHT11"),
        (670, 830, "OLED"),
        (890, 1050, "触摸模块"),
        (1110, 1270, "蜂鸣器"),
    ]
    for mx0, mx1, name in mods:
        box = (mx0, 1030, mx1, 1160)
        rounded(d, box, 10, MOD_FILL, MOD_LINE, 2)
        text(d, ((mx0 + mx1) // 2, 1052), name, f_mod, INK, "ma")
        text(d, ((mx0 + mx1) // 2, 1082), "VCC → + 轨", f_small, MUTED, "ma")
        text(d, ((mx0 + mx1) // 2, 1104), "GND → - 轨", f_small, MUTED, "ma")
        cx = (mx0 + mx1) // 2
        power_pair(cx, box[1], box[3])

    # ---- 信号线颜色图例（放在右下空白区）----
    legend = (1310, 1000, 1600, 1180)
    rounded(d, legend, 8, (248, 249, 250), (222, 226, 230), 1)
    text(d, (1324, 1012), "信号线颜色", f_legend, MUTED, "la")
    items = [
        (C_SIGNAL, "单总线数据", False),
        (C_TOUCH, "触摸输入", False),
        (C_BUZZER, "蜂鸣器", False),
        (C_SCL, "I2C SCL", False),
        (C_SDA, "I2C SDA", False),
        (C_SWD, "SWD 调试", False),
        (C_UART, "串口（可选）", True),
    ]
    for i, (color, label, dashed) in enumerate(items):
        y = 1038 + i * 20
        if dashed:
            wire(d, (1324, y), (1356, y), color, dash=True, width=4)
        else:
            d.line([(1324, y), (1356, y)], fill=color, width=4)
        text(d, (1366, y), label, f_legend, INK, "lm")

    text(d, (60, 1252),
         "注意：只用 ST-Link 供电时板子的 5V 引脚没有电，模块一律接 3V3（+ 轨）", f_note,
         (198, 40, 40), "la")

    img.save(out)
    print("saved:", out)


if __name__ == "__main__":
    main()
