#!/bin/sh
# run.sh — tests cible (headless) d'AsteroNeo
#
#   tests/run.sh ref     régénère les captures de référence (tests/ref/*.ppm)
#   tests/run.sh check   compare les captures courantes aux références (diff bit-à-bit),
#                        vérifie la cadence sous latence API mesurée en co-sim,
#                        et fait un test de fumée dans l'émulateur officiel neo.
#
# Oracle : Phosphoneo (déterministe : cycles, frappe automatique, capture PPM).
set -u
cd "$(dirname "$0")/.."
PHOS=${PHOSPHONEO:-$HOME/Phosphoneo/build/phosphoneo}
NEO=${NEO_EMU:-$HOME/Neo6502firmware/bin/neo}
LAT=tests/latency/api-latency-cosim.txt
OUT=tests/out; REF=tests/ref
mkdir -p "$OUT" "$REF"
mode=${1:-check}
fail=0

[ -x "$PHOS" ] || { echo "SKIP: Phosphoneo absent ($PHOS)"; exit 0; }
[ -f build/asteroneo.neo ] || { echo "FAIL: build/asteroneo.neo absent (make)"; exit 1; }

# Scénarios : nom | frappe | capture à (l'émulation s'arrête 10 000 cycles après)
scenario() {
    name=$1; keys=$2; at=$3; cycles=$((at + 10000))
    if [ -n "$keys" ]; then
        "$PHOS" build/asteroneo.neo --cycles "$cycles" --type-keys "$keys" \
            --screenshot-at "$at:$OUT/$name.ppm" >"$OUT/$name.log" 2>&1
    else
        "$PHOS" build/asteroneo.neo --cycles "$cycles" \
            --screenshot-at "$at:$OUT/$name.ppm" >"$OUT/$name.log" 2>&1
    fi
    if [ "$mode" = ref ]; then
        cp "$OUT/$name.ppm" "$REF/$name.ppm"; echo "REF  $name"
    elif cmp -s "$OUT/$name.ppm" "$REF/$name.ppm"; then
        echo "PASS $name"
    else
        echo "FAIL $name (capture != $REF/$name.ppm)"; fail=1
    fi
}

scenario title    ""            40000000
scenario game     "20000000: "  60000000
scenario controls "20000000:k"  50000000

# Programmes de test cible : primitives (t_line), idempotence XOR (t_xor),
# retour à NeoBASIC à la sortie (t_exit → PRINT 6*7 = 42).
prog_shot() {
    name=$1; at=$2
    "$PHOS" "build/$name.neo" --cycles $((at + 10000)) --screenshot-at "$at:$OUT/$name.ppm" >"$OUT/$name.log" 2>&1
    if [ "$mode" = ref ]; then cp "$OUT/$name.ppm" "$REF/$name.ppm"; echo "REF  $name"
    elif cmp -s "$OUT/$name.ppm" "$REF/$name.ppm"; then echo "PASS $name"
    else echo "FAIL $name (capture != $REF/$name.ppm)"; fail=1; fi
}
prog_shot t_line 2000000
prog_shot t_xor  3000000
"$PHOS" build/t_exit.neo --cycles 30000000 --type-keys '10000000:PRINT 6*7\n' \
    --screenshot-text "$OUT/t_exit.txt" >"$OUT/t_exit.log" 2>&1
if grep -q "^42" "$OUT/t_exit.txt"; then echo "PASS t_exit (retour NeoBASIC, 6*7 = 42)"
else echo "FAIL t_exit (NeoBASIC ne répond pas après la sortie)"; fail=1; fi

# Cadence : 255 pas de jeu sous latence API co-sim, ≤ 5 pas en retard.
addr=$(awk '/\._dbg_frames$/ {print $2}' build/asteroneo.lbl)
if [ -n "$addr" ]; then
    "$PHOS" build/asteroneo.neo --cycles 120000000 --type-keys '20000000: ' \
        --api-latency "$LAT" --dump-ram-when "${addr#00}:FF:$OUT/ram.bin" >"$OUT/cadence.log" 2>&1
    late=$(python3 -c "
import sys
a=int('$addr',16); d=open('$OUT/ram.bin','rb').read()
print(d[a+2] | (d[a+3] << 8))")
    if [ "$late" -le 5 ]; then echo "PASS cadence (pas en retard : $late / 255)"
    else echo "FAIL cadence (pas en retard : $late / 255)"; fail=1; fi
else
    echo "FAIL cadence : symbole _dbg_frames absent"; fail=1
fi

# Fumée dans l'émulateur officiel (CPU différent : pas de diff bit-à-bit).
if [ -x "$NEO" ]; then
    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 120 "$NEO" build/asteroneo.bin@800 cold \
        cycles:40000000 "shot:39000000:$OUT/neo.ppm" >"$OUT/neo.log" 2>&1
    n=$(python3 -c "
d=open('$OUT/neo.ppm','rb').read(); px=d.split(b'\n',3)[3]
print(sum(1 for i in range(0,len(px),3) if px[i]))" 2>/dev/null || echo 0)
    if [ "$n" -gt 200 ]; then echo "PASS neo (pixels allumés : $n)"
    else echo "FAIL neo (pixels allumés : $n)"; fail=1; fi
else
    echo "SKIP neo absent"
fi

[ $fail -eq 0 ] && echo "run.sh : OK" || echo "run.sh : ÉCHEC"
exit $fail
