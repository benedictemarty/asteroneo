#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
gen_ship.py — Tables précalculées du vaisseau (port Neo6502 d'Astéroric).

Produit src/ship_verts.c :
  ship_pt0..4 x/y [32] : les 5 sommets arcade (apex, arrière G/D, cockpit
                         G/D) après rotation, en pixels signés ;
  ship_thrx/thry [32]  : vecteur de poussée (amplitude TMAG px/frame).

Convention : angle 0 = pointe vers le HAUT (y vers le bas), angle croissant
= rotation horaire, 32 pas de 11,25°. Le vaisseau est agrandi de 20 % par
rapport à l'Oric (240×200 → 320×240) : apex à 6 px du centre.
"""

import math

N = 32
TMAG = 6

VERTS = [
    (0,  -6),    # P0 : pointe avant (apex)
    (-5,  5),    # P1 : arrière gauche extérieur
    (5,   5),    # P2 : arrière droit extérieur
    (-2,  0),    # P3 : cockpit gauche
    (2,   0),    # P4 : cockpit droit
]


def rotate(px, py, i):
    theta = 2.0 * math.pi * i / N
    rx = px * math.cos(theta) - py * math.sin(theta)
    ry = px * math.sin(theta) + py * math.cos(theta)
    return (int(round(rx)), int(round(ry)))


def emit(name, vals):
    print(f"const signed char {name}[{N}] = {{")
    for j in range(0, N, 8):
        print("    " + ", ".join(f"{v:3d}" for v in vals[j:j+8]) + ",")
    print("};")


def main():
    print("/* ship_verts.c — généré par tools/gen_ship.py (make gen_ship). NE PAS ÉDITER. */")
    print(f"/* {N} angles, {360.0/N:.3f}° par pas ; sommets {VERTS} ; thrust ±{TMAG} px/frame */")
    print()
    for (px, py), name in zip(VERTS, ("pt0", "pt1", "pt2", "pt3", "pt4")):
        emit(f"ship_{name}x", [rotate(px, py, i)[0] for i in range(N)])
        emit(f"ship_{name}y", [rotate(px, py, i)[1] for i in range(N)])
    emit("ship_thrx", [int(round(TMAG * math.sin(2.0 * math.pi * i / N))) for i in range(N)])
    emit("ship_thry", [int(round(-TMAG * math.cos(2.0 * math.pi * i / N))) for i in range(N)])


if __name__ == "__main__":
    main()
