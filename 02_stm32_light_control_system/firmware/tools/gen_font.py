#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
字库生成脚本（8x16 ASCII + 16x16 中文子集）

用法（在 firmware 目录下执行）：
    python tools/gen_font.py                 # 生成 Core/Src/oled_font_ascii.c 与 oled_font_cn.c
    python tools/gen_font.py --preview docs/ui-preview.png

中文子集来自 Core/Src/*.c 里的字符串常量，新增中文提示后重新运行即可。
ASCII 字库固定为 0x20~0x7E 共 95 个字符。
"""

import argparse
import os
import re
import sys
from glob import glob

from PIL import Image, ImageDraw, ImageFont

FW_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CORE_DIR = os.path.join(FW_DIR, "Core")
OUT_DIR = os.path.join(CORE_DIR, "Src")

FONT_ASCII = r"C:\Windows\Fonts\consola.ttf"
FONT_CN = r"C:\Windows\Fonts\simhei.ttf"

ASCII_SIZE = 14
ASCII_Y_OFFSET = 1
CN_SIZE = 16
THRESHOLD = 128

STRING_RE = re.compile(r'"((?:[^"\\]|\\.)*)"')


def load_font(path, size):
    if not os.path.exists(path):
        sys.exit("找不到字体文件: %s" % path)
    return ImageFont.truetype(path, size)


def collect_chars():
    """从 Core/Src 的字符串常量中收集非 ASCII 字符。"""
    chars = set()
    for path in sorted(glob(os.path.join(CORE_DIR, "Src", "*.c"))):
        with open(path, encoding="utf-8") as fp:
            for line in fp:
                for match in STRING_RE.finditer(line):
                    for ch in match.group(1):
                        if ord(ch) > 0x7E:
                            chars.add(ch)
    return sorted(chars, key=ord)


def render_cell(ch, font, cell_w, cell_h, y_offset=0):
    img = Image.new("L", (cell_w, cell_h), 0)
    draw = ImageDraw.Draw(img)
    draw.text((0, y_offset), ch, font=font, fill=255)
    return img


def bits_of(img, threshold=THRESHOLD):
    px = img.load()
    return [[1 if px[x, y] >= threshold else 0 for x in range(img.width)]
            for y in range(img.height)]


def ascii_rows(font):
    """返回 {codepoint: [16 字节]}"""
    table = {}
    for code in range(0x20, 0x7F):
        img = render_cell(chr(code), font, 8, 16, ASCII_Y_OFFSET)
        rows = bits_of(img)
        data = []
        for row in rows:
            byte = 0
            for col in range(8):
                if row[col]:
                    byte |= 0x80 >> col
            data.append(byte)
        table[code] = data
    return table


def cn_rows(chars, font):
    """返回 {codepoint: [32 字节]}"""
    table = {}
    for ch in chars:
        img = render_cell(ch, font, 16, 16, 0)
        rows = bits_of(img)
        data = []
        for row in rows:
            hi = 0
            lo = 0
            for col in range(8):
                if row[col]:
                    hi |= 0x80 >> col
                if row[col + 8]:
                    lo |= 0x80 >> col
            data.extend([hi, lo])
        table[ord(ch)] = data
    return table


def fmt_bytes(values):
    return ", ".join("0x%02X" % v for v in values)


def write_ascii(table):
    lines = [
        "/**",
        "  ******************************************************************************",
        "  * @file    oled_font_ascii.c",
        "  * @brief   8x16 ASCII 点阵字库（0x20~0x7E）",
        "  *",
        "  * 由 tools/gen_font.py 自动生成，请勿手工修改。",
        "  ******************************************************************************",
        "  */",
        '#include "oled_font.h"',
        "",
        "const uint8_t oled_font8x16[OLED_ASCII_LAST - OLED_ASCII_FIRST + 1][16] =",
        "{",
    ]
    for code in range(0x20, 0x7F):
        data = table[code]
        ch = chr(code)
        label = "SPACE" if ch == " " else ("'%s'" % ch if ch not in "\\*/" else repr(ch))
        lines.append("  { %s },  /* 0x%02X %s */" % (fmt_bytes(data), code, label))
    lines.append("};")
    lines.append("")
    path = os.path.join(OUT_DIR, "oled_font_ascii.c")
    with open(path, "w", encoding="utf-8", newline="\n") as fp:
        fp.write("\n".join(lines))
    return path


def write_cn(chars, table):
    lines = [
        "/**",
        "  ******************************************************************************",
        "  * @file    oled_font_cn.c",
        "  * @brief   16x16 中文点阵字库子集",
        "  *",
        "  * 由 tools/gen_font.py 自动生成，请勿手工修改。",
        "  * 字符来源：Core/Src 下 .c 文件的字符串常量，共 %d 个。" % len(chars),
        "  ******************************************************************************",
        "  */",
        '#include "oled_font.h"',
        "",
    ]
    for idx, ch in enumerate(chars):
        data = table[ord(ch)]
        lines.append("/* '%s' U+%04X */" % (ch, ord(ch)))
        lines.append("static const uint8_t cn_%02d[32] =" % idx)
        lines.append("{")
        for i in range(0, 32, 8):
            lines.append("  " + ", ".join("0x%02X" % v for v in data[i:i + 8]) + ",")
        lines.append("};")
        lines.append("")

    lines.append("const oled_cn_glyph_t oled_cn_table[] =")
    lines.append("{")
    for idx, ch in enumerate(chars):
        lines.append("  { 0x%04X, cn_%02d },  /* %s */" % (ord(ch), idx, ch))
    lines.append("};")
    lines.append("")
    lines.append("const uint16_t oled_cn_count = %d;" % len(chars))
    lines.append("")

    path = os.path.join(OUT_DIR, "oled_font_cn.c")
    with open(path, "w", encoding="utf-8", newline="\n") as fp:
        fp.write("\n".join(lines))
    return path


# --------------------------------------------------------------------------
# 界面预览：用与固件完全相同的点阵数据渲染 128x64 屏幕
# --------------------------------------------------------------------------

W, H = 128, 64


class Screen:
    def __init__(self):
        self.px = [[0] * W for _ in range(H)]

    def text(self, x, y, s, ascii_tbl, cn_tbl):
        for ch in s:
            if ord(ch) < 0x80:
                data = ascii_tbl.get(ord(ch))
                if data is None:
                    data = ascii_tbl[ord("?")]
                for row in range(16):
                    bits = data[row]
                    for col in range(8):
                        if bits & (0x80 >> col):
                            self.point(x + col, y + row)
                x += 8
            else:
                data = cn_tbl.get(ord(ch))
                if data is None:
                    x += 16
                    continue
                for row in range(16):
                    bits = (data[row * 2] << 8) | data[row * 2 + 1]
                    for col in range(16):
                        if bits & (0x8000 >> col):
                            self.point(x + col, y + row)
                x += 16

    def point(self, x, y):
        if 0 <= x < W and 0 <= y < H:
            self.px[y][x] = 1

    def width(self, s):
        return sum(8 if ord(ch) < 0x80 else 16 for ch in s)

    def text_center(self, y, s, ascii_tbl, cn_tbl):
        self.text(max(0, (W - self.width(s)) // 2), y, s, ascii_tbl, cn_tbl)

    def image(self):
        img = Image.new("L", (W, H), 0)
        for y in range(H):
            for x in range(W):
                if self.px[y][x]:
                    img.putpixel((x, y), 255)
        return img


def render_preview(ascii_tbl, cn_tbl, out_path):
    screens = []

    s = Screen()
    s.text(0, 0, "光照", ascii_tbl, cn_tbl)
    s.text(40, 0, "72%", ascii_tbl, cn_tbl)
    s.text(0, 16, "阈值", ascii_tbl, cn_tbl)
    s.text(40, 16, "30%", ascii_tbl, cn_tbl)
    s.text(0, 32, "模式", ascii_tbl, cn_tbl)
    s.text(40, 32, "自动", ascii_tbl, cn_tbl)
    s.text(0, 48, "灯", ascii_tbl, cn_tbl)
    s.text(24, 48, "关", ascii_tbl, cn_tbl)
    hint = "短按设置"
    s.text(128 - s.width(hint), 48, hint, ascii_tbl, cn_tbl)
    screens.append(("1. MAIN (AUTO)", s))

    s = Screen()
    s.text(0, 0, "光照", ascii_tbl, cn_tbl)
    s.text(40, 0, "18%", ascii_tbl, cn_tbl)
    s.text(0, 16, "阈值", ascii_tbl, cn_tbl)
    s.text(40, 16, "30%", ascii_tbl, cn_tbl)
    s.text(0, 32, "模式", ascii_tbl, cn_tbl)
    s.text(40, 32, "手动", ascii_tbl, cn_tbl)
    s.text(0, 48, "灯", ascii_tbl, cn_tbl)
    s.text(24, 48, "开", ascii_tbl, cn_tbl)
    hint = "短按开关灯"
    s.text(128 - s.width(hint), 48, hint, ascii_tbl, cn_tbl)
    screens.append(("2. MAIN (MANUAL)", s))

    s = Screen()
    s.text_center(0, "阈值设置", ascii_tbl, cn_tbl)
    s.text_center(16, "[30%]", ascii_tbl, cn_tbl)
    s.text_center(32, "短按+5%", ascii_tbl, cn_tbl)
    s.text_center(48, "长按返回", ascii_tbl, cn_tbl)
    screens.append(("3. SETTING", s))

    scale = 3
    gap = 18
    label_h = 16
    total_w = len(screens) * W + (len(screens) + 1) * gap
    total_h = H + label_h + 2 * gap
    canvas = Image.new("RGB", (total_w * scale // 1, total_h * scale // 1), (24, 26, 32))
    draw = ImageDraw.Draw(canvas)
    label_font = ImageFont.truetype(r"C:\Windows\Fonts\consola.ttf", 12)

    x = gap
    for label, scr in screens:
        card = Image.new("RGB", (W, H), (0, 0, 0))
        mono = scr.image()
        for y in range(H):
            for xx in range(W):
                if mono.getpixel((xx, y)):
                    card.putpixel((xx, y), (235, 245, 255))
        big = card.resize((W * scale, H * scale), Image.NEAREST)
        canvas.paste(big, (x * scale, (gap + label_h) * scale))
        draw.text((x * scale, gap * scale - 2), label, font=label_font, fill=(170, 180, 200))
        x += W + gap

    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    canvas.save(out_path)
    return out_path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--preview", help="输出 128x64 界面预览 PNG")
    args = parser.parse_args()

    ascii_font = load_font(FONT_ASCII, ASCII_SIZE)
    cn_font = load_font(FONT_CN, CN_SIZE)

    ascii_tbl = ascii_rows(ascii_font)
    chars = collect_chars()
    if not chars:
        sys.exit("没有找到中文字符，请检查 Core/Src 下的字符串")
    cn_tbl = cn_rows(chars, cn_font)

    p1 = write_ascii(ascii_tbl)
    p2 = write_cn(chars, cn_tbl)
    print("生成:", p1)
    print("生成:", p2)
    print("中文个数:", len(chars), "->", "".join(chars))

    if args.preview:
        out = render_preview(ascii_tbl, cn_tbl, os.path.abspath(args.preview))
        print("预览:", out)


if __name__ == "__main__":
    main()

