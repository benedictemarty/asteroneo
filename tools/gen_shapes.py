#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
gen_shapes.py — Formes vectorielles des astéroïdes (port Neo6502 d'Astéroric).

Source : les 4 formes Atari arcade rev 4 (AstPtrnPtrTbl $51DE →
$51E6/$51FE/$521A/$5234, format SVEC cumulé), identiques à la version Oric.

Sur Oric les silhouettes étaient pré-rasterisées en bitmaps (blit XOR).
Sur Neo6502 le tracé de ligne est fait par le RP2040 : on émet les
polygones fermés (sommets en pixels signés) et le runtime trace N segments
semi-ouverts par astéroïde. Échelles agrandies de 20 % (320×240).

Produit src/shapes.c :
  shape_nverts[12]          : nombre de sommets, index = size*4 + shape
  shape_vx[12][14], shape_vy : sommets (polygone fermé)
  shape_radii[3]            : rayon de collision L∞ par taille
"""

# Shape 0 ($51E6) — 11 sommets
ATARI_SHAPE_0 = [
    (0, 1), (1, 2), (2, 1), (1, -1), (2, -3),
    (-1, -5), (-4, -5), (-5, -4), (-5, -2),
    (-4, -1), (-3, -2),
]
# Shape 1 ($51FE) — 13 sommets
ATARI_SHAPE_1 = [
    (2, 1), (4, 2), (3, 3), (1, 2), (-1, 3),
    (-2, 2), (-1, 0), (-2, -2), (-1, -3),
    (0, -2), (3, -3), (5, 0), (4, 1),
]
# Shape 2 ($521A) — 12 sommets
ATARI_SHAPE_2 = [
    (-1, 0), (-3, -1), (-1, -4), (1, -1), (1, -4),
    (2, -4), (4, -1), (4, 0), (2, 3),
    (-1, 3), (-4, 0), (-2, -1),
]
# Shape 3 ($5234) — 13 sommets
ATARI_SHAPE_3 = [
    (1, 0), (4, 1), (4, 2), (1, 4), (-2, 4),
    (-1, 2), (-4, 2), (-4, -1), (-2, -4),
    (1, -3), (2, -4), (3, -3), (0, -1),
]

ATARI_SHAPES = [ATARI_SHAPE_0, ATARI_SHAPE_1, ATARI_SHAPE_2, ATARI_SHAPE_3]
SCALES = [1.1, 1.9, 3.1]    # petit, moyen, grand (Oric : 0.9 / 1.6 / 2.6)
MAXV = 14


def scaled(shape, s):
    return [(int(round(x * s)), int(round(y * s))) for (x, y) in shape]


def main():
    print("/* shapes.c — généré par tools/gen_shapes.py (make gen_shapes). NE PAS ÉDITER. */")
    print(f"/* 4 formes Atari rev 4 × 3 tailles, échelles {SCALES} */")
    print()
    print(f"#define SHAPE_MAXV {MAXV}")
    print()
    nv, vx, vy, radii = [], [], [], []
    for s in SCALES:
        r = 0
        for shape in ATARI_SHAPES:
            pts = scaled(shape, s)
            nv.append(len(pts))
            vx.append([p[0] for p in pts] + [0] * (MAXV - len(pts)))
            vy.append([p[1] for p in pts] + [0] * (MAXV - len(pts)))
            r = max(r, max(max(abs(x), abs(y)) for (x, y) in pts))
        radii.append(r)
    print("const unsigned char shape_nverts[12] = { " + ", ".join(str(n) for n in nv) + " };")
    print()
    for name, tab in (("shape_vx", vx), ("shape_vy", vy)):
        print(f"const signed char {name}[12][SHAPE_MAXV] = {{")
        for row in tab:
            print("    { " + ", ".join(f"{v:3d}" for v in row) + " },")
        print("};")
        print()
    print("const unsigned char shape_radii[3] = { " + ", ".join(str(r) for r in radii) + " };")


if __name__ == "__main__":
    main()
