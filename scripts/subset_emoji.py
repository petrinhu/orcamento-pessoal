#!/usr/bin/env python3
"""Subset da Noto Color Emoji para SÓ os emoji usados no código.

Varre src/**.{cpp,h}, coleta os caracteres que existem na Noto Color Emoji
(emoji de verdade), e gera resources/fonts/NotoColorEmoji.ttf contendo apenas
esses glifos (+ FE0F). Mantém COLR/CPAL (cor). Roda de novo sempre que um emoji
novo for adicionado ao código.

Uso: python3 scripts/subset_emoji.py
Requer: fonttools (pyftsubset) e a Noto Color Emoji do sistema.
"""
import glob
import subprocess
import sys
from fontTools.ttLib import TTFont

SYS_FONT = "/usr/share/fonts/google-noto-color-emoji-fonts/Noto-COLRv1.ttf"
OUT = "resources/fonts/NotoColorEmoji.ttf"

full = TTFont(SYS_FONT)
emoji_cmap = set(full.getBestCmap().keys())

used = set()
for path in glob.glob("src/**/*.cpp", recursive=True) + glob.glob("src/**/*.h", recursive=True):
    with open(path, encoding="utf-8") as fh:
        for ch in fh.read():
            cp = ord(ch)
            if cp >= 0x2190 and cp in emoji_cmap:
                used.add(cp)

used.add(0xFE0F)  # seletor de apresentacao emoji (cor)
codes = ",".join(f"{c:X}" for c in sorted(used))
print(f"{len(used)} codepoints -> {OUT}")

rc = subprocess.run([
    "pyftsubset", SYS_FONT,
    f"--unicodes={codes}",
    f"--output-file={OUT}",
]).returncode
if rc != 0:
    sys.exit(rc)

out = TTFont(OUT)
assert "COLR" in out and "CPAL" in out, "COLR/CPAL perdidos no subset!"
import os
print(f"OK: {os.path.getsize(OUT)/1024:.1f} KB, COLR/CPAL preservados")
