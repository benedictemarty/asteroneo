"""
gen_shapes.py — Formes vectorielles des astéroïdes (port Neo6502 d'Astéroric).

Source : les 4 formes Atari arcade rev 4 (AstPtrnPtrTbl $51DE →
$51E6/$51FE/$521A/$5234, format SVEC cumulé), identiques à la version Oric.

Sur Neo6502 le tracé de ligne est fait par le RP2040 : le nombre de sommets
est libre. Les SVEC arcade vivent sur une grille entière de ±5 unités (1 unité
= 1 à 3 px ici), d'où des arêtes alignées sur les axes et des angles droits
peu naturels en pixels nets (retour playtest 2026-09-15). Chaque silhouette
Atari est donc DÉRIVÉE ainsi :
  1. rééchantillonnage régulier du contour (4 × N points) ;
  2. lissage (moyenne glissante sur 5 points, 2 passes) → coins arrondis ;
  3. décimation à N sommets (24 gros, 16 moyens, 10 petits), fusion des
     sommets à moins de 2 px ;
  4. rotation propre à chaque forme (13°, 29°, 41°, 23°) → plus d'arête sur
     un axe ; relief radial déterministe ±4 % (graine = size*4+shape).
La silhouette générale (bosses, encoches) reste celle d'Atari.

Produit src/shapes.c :
  shape_nverts[12]          : nombre de sommets, index = size*4 + shape
  shape_vx[12][24], shape_vy : sommets (polygone fermé)
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
NVERTS = [10, 16, 24]       # sommets par taille
ROT_DEG = [13, 29, 41, 23]  # rotation par forme
JITTER = 0.04
MIN_EDGE = 2.0              # arête minimale en px (fusion des sommets quasi confondus)
SMOOTH_PASSES = 2
MAXV = max(NVERTS)

import math
import random


def derive(shape, n, scale, seed, rot_deg):
    """Contour Atari → polygone lissé de n sommets (pixels signés)."""
    pts = [(x * scale, y * scale) for (x, y) in shape]
    segs, total = [], 0.0
    for i in range(len(pts)):
        a, b = pts[i], pts[(i + 1) % len(pts)]
        d = math.hypot(b[0] - a[0], b[1] - a[1])
        segs.append((a, b, d))
        total += d
    m = 4 * n
    dense = []
    for k in range(m):
        t, acc = (k / m) * total, 0.0
        for a, b, d in segs:
            if acc + d >= t:
                u = (t - acc) / d if d else 0.0
                dense.append((a[0] + (b[0] - a[0]) * u, a[1] + (b[1] - a[1]) * u))
                break
            acc += d
    for _ in range(SMOOTH_PASSES):
        dense = [(sum(dense[(i + k) % m][0] for k in range(-2, 3)) / 5,
                  sum(dense[(i + k) % m][1] for k in range(-2, 3)) / 5) for i in range(m)]
    rnd = random.Random(seed)
    th = math.radians(rot_deg)
    out = []
    for k in range(n):
        x, y = dense[k * 4]
        r = math.hypot(x, y) * (1 + rnd.uniform(-JITTER, JITTER))
        ang = math.atan2(y, x) + th
        out.append((int(round(r * math.cos(ang))), int(round(r * math.sin(ang)))))
    # Fusion des sommets à moins de MIN_EDGE px du précédent : deux segments
    # de 2 px se recouvrent en XOR (pixel éteint au sommet).
    kept = []
    for pt in out:
        if kept and math.hypot(pt[0] - kept[-1][0], pt[1] - kept[-1][1]) < MIN_EDGE:
            continue
        kept.append(pt)
    if len(kept) > 1 and math.hypot(kept[0][0] - kept[-1][0], kept[0][1] - kept[-1][1]) < MIN_EDGE:
        kept.pop()
    return kept


def main():
    print("/* shapes.c — généré par tools/gen_shapes.py (make gen_shapes). NE PAS ÉDITER. */")
    print(f"/* 4 formes Atari rev 4 lissées × 3 tailles, échelles {SCALES}, sommets {NVERTS} */")
    print()
    print(f"#define SHAPE_MAXV {MAXV}")
    print()
    nv, vx, vy, radii = [], [], [], []
    for size, s in enumerate(SCALES):
        r = 0
        for sh, shape in enumerate(ATARI_SHAPES):
            pts = derive(shape, NVERTS[size], s, size * 4 + sh, ROT_DEG[sh])
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
