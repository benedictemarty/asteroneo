# AsteroNeo — portage Neo6502 d'Astéroric

**AsteroNeo** est le portage sur **Neo6502** (Olimex : W65C02S à 6,25 MHz +
RP2040) d'[Astéroric](https://github.com/benedictemarty/oric-asteroids), clone
d'étude d'*Asteroids* arcade (Atari, 1979) écrit pour l'Oric-1 48 Ko.

La logique de jeu (transposée du désassemblage arcade rev 4 — vagues,
fragmentation, IA de la soucoupe, hyperespace, high scores) est reprise
telle quelle en C cc65 ; tout ce qui touchait le matériel Oric (HIRES,
VIA, AY-3-8912) est remplacé par l'**API du firmware Neo6502** :

| Oric-1 (Astéroric)                          | Neo6502 (AsteroNeo)                                   |
|---------------------------------------------|--------------------------------------------------------|
| HIRES 240×200 mono, Bresenham XOR maison    | mode 0 320×240, lignes/pixels XOR de l'API (5,1 / 5,2 / 5,5) |
| astéroïdes en bitmaps pré-rendus (blit)     | polygones vectoriels arcade (11-13 sommets) tracés par le RP2040 |
| clavier : matrice VIA/PSG                   | Key Status par code HID (1,2), touches configurables  |
| AY-3-8912 sous IRQ Timer 1                  | générateur 4 canaux carré/bruit, files de notes (8,7) |
| VSync/Timer 1, 25 Hz                        | compteur de trames 60 Hz (5,37), pas de jeu à 30 Hz   |
| coordonnées 8 bits                          | coordonnées `int` (320 > 255), intégration 8.8 avec retenue |

Le programme n'utilise que l'API standard du firmware amont (mode 0) : il
tourne sur toute carte Neo6502, dans l'émulateur officiel `neo` et dans
[Phosphoneo](../Phosphoneo) (oracle des tests).

## Build

Prérequis : `cc65` 2.19 (cl65/ca65/ld65), Python 3, et pour les tests
`gcc`, Phosphoneo (`~/Phosphoneo/build/phosphoneo`) et l'émulateur `neo`
(`~/Neo6502firmware/bin/neo`).

```
make            # build/asteroneo.bin (brut, $0800) et build/asteroneo.neo
make run        # émulateur officiel neo (fenêtre SDL, touches actives)
make run-phos   # Phosphoneo headless, capture tests/out/phos.ppm
make test       # tests host (gcc) + captures cible vs références + cadence + fumée neo
make ref        # régénère les captures de référence (après un changement voulu)
make gen        # régénère src/ship_verts.c et src/shapes.c
make clean
```

Le `.neo` se charge sur la carte depuis la carte SD/clé USB (`run "asteroneo.neo"`
dans NeoBASIC) ; en émulation : `neo asteroneo.bin@800 cold` ou
`phosphoneo asteroneo.neo`.

## Touches

| Touche  | Action                                   |
|---------|------------------------------------------|
| `←`     | Rotation gauche                          |
| `→`     | Rotation droite                          |
| `↑`     | Poussée (thrust)                         |
| `↓`     | Hyperespace (téléportation, 25 % de mort) |
| `SPACE` | Tir / démarrer / rejouer                 |
| `K`     | (écran titre) configuration des touches  |
| `ESC`   | Quitter (game over) / annuler (config)   |

Les 5 actions sont remappables sur n'importe quelle touche nommée (A-Z,
0-9, pavé numérique, flèches, F1-F12…) ; le mapping vit en RAM.

## Architecture

```
src/game.c        boucle de jeu, ship, torpilles, collisions, game over, jingle
src/asteroids.c   vagues, fragmentation arcade, rendu polygonal + wraparound
src/ufo.c         soucoupe grande/petite, IA de tir
src/hud.c         score 7 segments, vies
src/font.c        police vectorielle A-Z 0-9 ; src/title.c labels
src/keys.c        écran CONTROLS
src/phys.c        intégration 8.8 (retenue) et collision torique
src/asm/neo_gfx.s primitives XOR : gfx_init, draw_line_xor(_open), plot_dot
src/neo_time.c    frame_tick / vsync_wait (5,37)
src/neo_input.c   key_scan / key_probe / key_map (1,2), noms HID
src/neo_sound.c   effets FX_* et jingle sur l'API son
src/shapes.c, src/ship_verts.c   tables générées (tools/gen_*.py)
cfg/neo6502.cfg   RAM $0800-$FBFF, pile C 1 Ko sous le noyau ($FC00)
```

Point d'attention : la ligne du firmware (EFLA) est **semi-ouverte**
(`[P0, P1[`) ; `neo_gfx.s` en fait une ligne fermée (ligne + pixel
d'arrivée) et une ligne « ouverte » `]P0, P1]` (tracée à l'envers) pour
que chaque sommet d'un polygone soit XOR-é exactement une fois.

cc65 2.19 est compilé avec `--disable-opt OptStackOps` (bug d'optimisation,
cf. [docs/cc65-optstackops.md](docs/cc65-optstackops.md)).

## Tests

- `tests/host/` : logique portable compilée avec gcc contre des stubs qui
  reproduisent l'EFLA du firmware (intégration 8.8, collisions, tables des
  formes, idempotence XOR, wraparound, fragmentation, table des touches).
- `tests/run.sh` : scénarios Phosphoneo déterministes (titre, partie,
  écran CONTROLS) comparés bit à bit à `tests/ref/*.ppm` ; cadence sous
  la latence API mesurée en co-simulation sur le vrai firmware
  (`tests/latency/api-latency-cosim.txt`) ; test de fumée dans `neo`.

Voir [ROADMAP.md](ROADMAP.md) (plan agile) et [CHANGELOG.md](CHANGELOG.md).

## Licence

Code original sous **EUPL v1.2** ([LICENSE](LICENSE)). La logique de jeu
adaptée du désassemblage Atari et les formes extraites de la ROM ne sont
pas couvertes : voir [NOTICE.md](NOTICE.md). Projet d'étude et de
préservation, non commercial, non affilié à Atari.
