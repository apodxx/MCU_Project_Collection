#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Render the six OLED faces of the corrugated robot.

The drawing code mirrors the Adafruit_GFX primitives used in
corrugated_robot.ino, so this file doubles as a quick layout preview:
run it after changing eye/mouth coordinates.

Usage:
    python render_faces.py [output.png]
"""

from __future__ import annotations

import math
import os
import sys

from PIL import Image, ImageDraw, ImageFont

W, H = 128, 64
SCALE = 5
INK = (255, 255, 255)
BG = (0, 0, 0)

FONT_CN = r"C:\Windows\Fonts\simhei.ttf"
FONT_EN = r"C:\Windows\Fonts\consolab.ttf"


class Canvas:
    """A tiny 1-bit canvas with the GFX calls used by the sketch."""

    def __init__(self) -> None:
        self.image = Image.new("L", (W, H), 0)
        self.draw = ImageDraw.Draw(self.image)

    def fill_rect(self, x: int, y: int, w: int, h: int, color: int = 255) -> None:
        self.draw.rectangle([x, y, x + w - 1, y + h - 1], fill=color)

    def fill_round_rect(self, x: int, y: int, w: int, h: int, r: int, color: int = 255) -> None:
        self.draw.rounded_rectangle([x, y, x + w - 1, y + h - 1], radius=r, fill=color)

    def fill_circle(self, cx: int, cy: int, r: int, color: int = 255) -> None:
        self.draw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=color)

    def circle(self, cx: int, cy: int, r: int, color: int = 255, width: int = 1) -> None:
        self.draw.ellipse([cx - r, cy - r, cx + r, cy + r], outline=color, width=width)

    def line(self, x0: int, y0: int, x1: int, y1: int, color: int = 255, width: int = 1) -> None:
        self.draw.line([x0, y0, x1, y1], fill=color, width=width)

    def triangle(self, points, color: int = 255) -> None:
        self.draw.polygon(points, fill=color)

    def arc_thick(self, cx: int, cy: int, r: int, a0: int, a1: int, thickness: int) -> None:
        for deg in range(a0, a1 + 1, 3):
            rad = math.radians(deg)
            x = round(cx + math.cos(rad) * r)
            y = round(cy + math.sin(rad) * r)
            self.fill_circle(x, y, thickness // 2)


def heart(c: Canvas, cx: int, cy: int, size: int) -> None:
    r = max(2, size // 4)
    c.fill_circle(cx - r, cy - r // 2, r)
    c.fill_circle(cx + r, cy - r // 2, r)
    c.triangle([(cx - size // 2, cy - r // 2 + 1),
                (cx + size // 2, cy - r // 2 + 1),
                (cx, cy + size // 2)])


def eye_open(c: Canvas, cx: int, cy: int, w: int = 20, h: int = 24) -> None:
    c.fill_round_rect(cx - w // 2, cy - h // 2, w, h, 6)
    # black pupil + one white catch-light
    c.fill_round_rect(cx - 4, cy - 5, 9, 13, 4, 0)
    c.fill_rect(cx - 2, cy - 3, 3, 3, 255)


def eye_happy(c: Canvas, cx: int, cy: int) -> None:
    c.arc_thick(cx, cy + 4, 10, 200, 340, 5)


def face_happy(c: Canvas) -> None:
    eye_happy(c, 42, 23)
    eye_happy(c, 86, 23)
    c.arc_thick(64, 37, 17, 20, 160, 4)
    c.line(23, 45, 31, 45, width=2)
    c.line(97, 45, 105, 45, width=2)


def face_curious(c: Canvas) -> None:
    eye_open(c, 42, 25, 20, 25)
    eye_open(c, 87, 24, 17, 21)
    c.arc_thick(64, 43, 10, 25, 135, 3)


def face_surprised(c: Canvas) -> None:
    for cx in (42, 86):
        c.circle(cx, 24, 11, width=3)
        c.fill_circle(cx, 25, 4)
        c.fill_circle(cx - 1, 24, 1)
    c.circle(64, 49, 7, width=3)
    c.fill_circle(64, 49, 2)


def face_sleepy(c: Canvas) -> None:
    c.arc_thick(42, 22, 10, 25, 155, 4)
    c.arc_thick(86, 22, 10, 25, 155, 4)
    c.arc_thick(64, 50, 5, 0, 360, 3)
    # two little Zs
    for x, y, s in ((100, 8, 2), (111, 2, 2)):
        c.line(x, y, x + 5 * s, y, width=s)
        c.line(x + 5 * s, y, x, y + 6 * s, width=s)
        c.line(x, y + 6 * s, x + 5 * s, y + 6 * s, width=s)


def face_love(c: Canvas) -> None:
    heart(c, 42, 24, 20)
    heart(c, 86, 24, 20)
    c.arc_thick(64, 39, 13, 25, 155, 4)


def face_angry(c: Canvas) -> None:
    c.line(30, 15, 52, 21, width=4)
    c.line(98, 15, 76, 21, width=4)
    for cx in (42, 86):
        c.fill_round_rect(cx - 8, 25, 16, 15, 4)
        c.fill_round_rect(cx - 3, 29, 6, 7, 2, 0)
    c.arc_thick(64, 53, 14, 200, 340, 4)


FACES = [
    ("开心 HAPPY", face_happy),
    ("好奇 CURIOUS", face_curious),
    ("惊讶 SURPRISED", face_surprised),
    ("喜欢 LOVE", face_love),
    ("困了 SLEEPY", face_sleepy),
    ("生气 ANGRY", face_angry),
]


def main() -> None:
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "..", "docs", "ui-preview.png")
    out = os.path.abspath(out)
    os.makedirs(os.path.dirname(out), exist_ok=True)

    margin, gap, header, footer = 44, 34, 82, 34
    cell_w, cell_h = W * SCALE, H * SCALE
    cols = 3
    rows = 2
    sheet_w = margin * 2 + cols * cell_w + (cols - 1) * gap
    sheet_h = header + rows * (cell_h + footer) + (rows - 1) * 22 + 56
    sheet = Image.new("RGB", (sheet_w, sheet_h), (247, 247, 244))
    d = ImageDraw.Draw(sheet)

    f_title = ImageFont.truetype(FONT_CN, 30)
    f_sub = ImageFont.truetype(FONT_CN, 16)
    f_label = ImageFont.truetype(FONT_CN, 19)
    f_note = ImageFont.truetype(FONT_CN, 15)
    f_en = ImageFont.truetype(FONT_EN, 14)

    d.text((sheet_w // 2, 24), "瓦楞机器人 · 0.96\" OLED 表情库", font=f_title,
           fill=(33, 37, 41), anchor="ma")
    d.text((sheet_w // 2, 60), "128 × 64 单色 · 每摸一次触摸键切换一个表情", font=f_sub,
           fill=(110, 117, 125), anchor="ma")

    for i, (label, draw_face) in enumerate(FACES):
        col, row = i % cols, i // cols
        x = margin + col * (cell_w + gap)
        y = header + row * (cell_h + footer + 22)
        panel = Canvas()
        draw_face(panel)
        rgb = Image.merge("RGB", (panel.image, panel.image, panel.image))
        rgb = rgb.resize((cell_w, cell_h), Image.Resampling.NEAREST)
        sheet.paste(rgb, (x, y))
        d.rectangle([x - 1, y - 1, x + cell_w, y + cell_h], outline=(38, 50, 56), width=2)
        d.text((x + cell_w // 2, y + cell_h + 8), label, font=f_label,
               fill=(33, 37, 41), anchor="ma")
        d.text((x + cell_w // 2, y + cell_h + 32), f"FACE {i}", font=f_en,
               fill=(142, 148, 156), anchor="ma")

    note_y = sheet_h - 42
    d.text((sheet_w // 2, note_y),
           "上电默认「开心」；触摸依次切换 好奇 → 惊讶 → 喜欢 → 困了 → 生气 → 开心",
           font=f_note, fill=(90, 98, 104), anchor="ma")

    sheet.save(out)
    print("saved:", out)


if __name__ == "__main__":
    main()
