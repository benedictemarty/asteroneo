# ROADMAP — AsteroNeo (portage Neo6502 d'Astéroric)

Projet mené en méthode agile : sprints courts, chaque sprint livre un
binaire testé (`make test`), la documentation et le CHANGELOG à jour, et
un commit par incrément.

## Décisions de cadrage (2026-09-15, PO bmarty)

| Sujet          | Décision                                                          |
|----------------|-------------------------------------------------------------------|
| Dépôt          | dépôt git dédié `~/Neo6502AsteroNeo` (identité bmarty <bmarty@mailo.com>) |
| Cible firmware | **firmware amont, mode 0** (320×240×256) — API standard uniquement |
| Terrain        | **plein écran 320×240**, constantes adaptées (wrap, spawn, HUD)   |
| Langage        | C cc65 (logique reprise d'Astéroric) + ca65 pour la couche API    |
| Cadence        | 30 Hz (2 trames de 60 Hz par pas) — constantes 25 Hz recalibrées  |
| Rendu          | vectoriel XOR par l'API (lignes semi-ouvertes gérées dans neo_gfx.s) |
| Émulateurs     | Phosphoneo = oracle des tests (déterministe) ; `neo` = fumée / jeu |

## Sprint 1 — 2026-09-15 — « Portage jouable » ✔

- [x] Dépôt, chaîne cc65 (`cfg/neo6502.cfg`, `crt0.s`), `.neo` (`tools/mkneo.py`)
- [x] Couche Neo6502 : XOR (`neo_gfx.s`), cadence (`neo_time.c`), clavier HID
      (`neo_input.c`), son 4 canaux (`neo_sound.c`)
- [x] Portage des 7 modules C en coordonnées `int` ; intégration 8.8 avec
      retenue (`phys.c`) ; tables générées (`tools/gen_ship.py`, `gen_shapes.py`)
- [x] Écran titre + jingle, démo, CONTROLS, partie complète, game over, high scores
- [x] Tests host (3), tests cible (3 captures de référence, cadence, fumée `neo`)
- [x] Bug cc65 `OptStackOps` identifié et contourné (`docs/cc65-optstackops.md`)
- [x] Latence API mesurée en co-simulation sur le vrai firmware :
      ligne 5,2 ≈ 22 cycles, pixel 5,5 ≈ 5 cycles → 3 pas en retard sur 255

## Sprint 2 — 2026-09-15 — « Playtest et polish » (en cours)

- [x] Titre ASTERONEO ; rotation du vaisseau 1 pas/frame (tour en 1,1 s)
- [x] Astéroïdes arrondis (formes Atari lissées, 24/16/10 sommets), tracé de
      polygone en asm (`poly_xor`) pour tenir la cadence

- [ ] Playtest interactif dans `neo` (touches, son, cadence ressentie) et
      calibrage des durées d'effets sonores à l'oreille
- [ ] Vérifier la sémantique des paramètres 8,7 (slide, volume) sur le
      firmware ; enveloppes plus fines si utile
- [ ] Manette USB (7,1 : flèches + boutons A/B) en plus du clavier
- [ ] Option 60 Hz (VSYNCS_PER_FRAME 1 + vitesses ÷ 2) si le rendu le permet
- [ ] Persistance des high scores et du mapping des touches (groupe 3, fichier
      sur SD/USB)
- [ ] Sons : chime FX_LIFE et hyperespace plus proches de l'arcade

## Sprint 3 — « Carte réelle »

- [ ] Test sur Neo6502 physique dès disponibilité (cadence, latence réelle
      des appels, rendu DVI, clavier USB) ; comparer à la co-sim
- [ ] Release : `dist/asteroneo.neo`, notice de chargement, capture

## Différé / idées

- Astéroïdes et vaisseau en sprites matériels (groupe 6) : supprimerait
  l'effacement XOR — inutile tant que la latence des lignes reste ≈ 22 cycles
- Couleurs (palette 256) : HUD et soucoupe en couleur, terrain monochrome
- Mode Hercules 720×350 du fork (hors périmètre : firmware amont seulement)

## Risques

| Risque | Mitigation |
|--------|------------|
| Latence réelle des appels API supérieure à la co-sim | compteurs `dbg_frames`/`dbg_late`, plan B sprites |
| Bugs de codegen cc65 2.19 | `--disable-opt OptStackOps`, tests de référence bit-à-bit |
| Sons approximatifs (pas d'enveloppe matérielle) | notes en escalier de volume ; calibrage sprint 2 |
