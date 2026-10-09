#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Dondji JA: 東雲 12 ドット（ゴシック, JIS X 0208:1990）から SPI Flash 用字庫 cn_font.bin を作る
#
# 出力の配置はF/Wの SETTINGS_CNCharToIndex / SETTINGS_ReadCNFontBitmap と同じ:
#   [ビットマップ N×24B][索引 N×4B (unicode<<16 | 字番号, Unicode 昇順)][拼音表 0B][版数 1B]
#   - ビットマップは 1 字 12 行 × uint16 (LE)、MSB が左端
#   - 拼音表は持たない（ENABLE_CN_PINYIN=OFF 前提）
#   - 先頭の字は「的」: F/W・Web ツールのプローブ値 (0x1100, 0x2100) を満たすため
#
# 対象: JIS X 0208 のうち U+3000〜U+FFFF（UTF-8 で 3 バイト、先頭 0xE3〜0xEF）の字
#       2 バイトになる字（ギリシャ・キリル・一部記号）はF/Wが扱えないので除く
#
# 使い方: python3 App/tools/gen_ja_font.py
#   → docs/font/cn_font.bin, docs/fonts/cn_font.bin を上書きし、settings.h / flash.js 用の定数を表示
#
# フォント: App/bdf/shinonome/shnmk12_font_src.bit（東雲フォント, Public Domain, App/bdf/shinonome/LICENSE）

import os
import re
import shutil
import struct
import sys

FONT_VERSION = 4          # 2: 公式 WenQuanYi, 3: WenQuanYi+JIS 試作, 4: 東雲のみ
PROBE_CHAR = '的'
PROBE_ROWS = (0x1100, 0x2100)


def load_shinonome(path):
    """.bit（BDF のビットマップを '.'/'@' で書いた形式）を読み、{文字: [12 行の uint16]} を返す"""
    text = open(path, encoding='utf-8').read()
    glyphs = {}
    pattern = re.compile(
        r'ENCODING (\d+)\n.*?BBX (\d+) (\d+) (-?\d+) (-?\d+)\nBITMAP\n(.*?)\nENDCHAR', re.S)
    for m in pattern.finditer(text):
        enc = int(m.group(1))
        width, height = int(m.group(2)), int(m.group(3))
        rows = m.group(6).split('\n')
        if width != 12 or height != 12 or len(rows) != 12:
            sys.exit('unexpected glyph size at ENCODING %d' % enc)
        try:
            ch = bytes([(enc >> 8) + 0x80, (enc & 0xFF) + 0x80]).decode('euc_jp')
        except UnicodeDecodeError:
            sys.exit('cannot map JIS 0x%04X to Unicode' % enc)
        bits = []
        for row in rows:
            if len(row) != 12 or any(c not in '.@' for c in row):
                sys.exit('bad bitmap row at ENCODING %d: %r' % (enc, row))
            value = 0
            for col, c in enumerate(row):
                if c == '@':
                    value |= 0x8000 >> col
            bits.append(value)
        glyphs[ch] = (enc, bits)
    return glyphs


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo = os.path.normpath(os.path.join(script_dir, '..', '..'))
    src = os.path.join(repo, 'App', 'bdf', 'shinonome', 'shnmk12_font_src.bit')

    glyphs = load_shinonome(src)
    print('Shinonome glyphs: %d' % len(glyphs))

    # JIS 区点順、U+3000〜U+FFFF のみ
    chars = sorted((c for c in glyphs if 0x3000 <= ord(c) <= 0xFFFF), key=lambda c: glyphs[c][0])
    skipped = len(glyphs) - len(chars)
    print('3-byte UTF-8 chars: %d (skipped 2-byte: %d)' % (len(chars), skipped))

    if PROBE_CHAR not in chars or tuple(glyphs[PROBE_CHAR][1][:2]) != PROBE_ROWS:
        sys.exit('probe glyph mismatch: %r' % (glyphs.get(PROBE_CHAR),))
    chars.remove(PROBE_CHAR)
    chars.insert(0, PROBE_CHAR)

    n = len(chars)
    bitmap = bytearray()
    index = []
    for i, ch in enumerate(chars):
        for v in glyphs[ch][1]:
            bitmap += struct.pack('<H', v)
        index.append((ord(ch), i))
    index.sort()

    bitmap_size = len(bitmap)
    index_size = n * 4
    py_offset = bitmap_size + index_size
    version_offset = py_offset          # 拼音表 0 バイト

    out = bytearray(bitmap)
    for u, i in index:
        out += struct.pack('<I', (u << 16) | i)
    assert len(out) == version_offset
    out += bytes([FONT_VERSION])

    for sub in ('font', 'fonts'):
        path = os.path.join(repo, 'docs', sub, 'cn_font.bin')
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'wb') as f:
            f.write(out)
        print('Wrote %s (%d bytes)' % (path, len(out)))

    print()
    print('--- App/settings.h ---')
    print('#define CN_FONT_CHAR_COUNT      %du' % n)
    print('#define CN_FONT_BITMAP_SIZE     %du' % bitmap_size)
    print('#define CN_FONT_INDEX_SIZE      %du' % index_size)
    print('#define CN_FONT_PY_OFFSET       %du' % py_offset)
    print('#define CN_FONT_PY_COUNT        0u')
    print('#define CN_FONT_VERSION         %du' % FONT_VERSION)
    print('#define CN_FONT_VERSION_OFFSET  %du' % version_offset)
    print('#define CN_FONT_PY_TOTAL_SIZE   0u')
    print('--- docs/js/flash.js ---')
    print('const CN_FONT_VERSION_OFFSET = %d;' % version_offset)
    print('const CN_FONT_BITMAP_SIZE = %d;' % bitmap_size)
    print('const CN_FONT_CHAR_COUNT = %d;' % n)
    print('const CN_FONT_VERSION     = %d;' % FONT_VERSION)


if __name__ == '__main__':
    main()
