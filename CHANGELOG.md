# CHANGELOG — AsteroNeo

Toutes les modifications notables du projet, par sprint et par date.

## [0.2.0] — 2026-09-15 — astéroïdes arrondis

### Modifié
- Formes des astéroïdes (retour playtest : angles droits peu naturels) :
  les 4 silhouettes Atari sont désormais dérivées par `tools/gen_shapes.py`
  — rééchantillonnage du contour, lissage (moyenne glissante), 24/16/10
  sommets (fusion des sommets < 2 px), rotation propre à chaque forme
  (13°/29°/41°/23°), relief ±4 %. Rayons 6/10/18. Tables `[12][24]`.
- Tracé de polygone en asm (`poly_xor`, `neo_gfx.s` : clip par segment,
  segments semi-ouverts) — la boucle C coûtait ~1 500 cycles/segment et
  passait à 200 pas en retard / 255 avec 24 sommets ; 2 / 255 désormais.
  Stub host équivalent dans `tests/host/stubs.c`.
- Tests : `test_shapes` (sommets ≤ 10/16/24, tolérance d'un sommet éteint
  par silhouette — recouvrement XOR de segments voisins), références
  régénérées (titre, partie, CONTROLS, t_xor).

## [0.1.1] — 2026-09-15

### Modifié
- Rotation du vaisseau : 1 pas (11,25°) par frame au lieu de 2 — tour complet
  en ≈ 1,1 s à 30 Hz (retour playtest : trop rapide).
- Écran titre : « ASTERONEO » (demande PO, ex-« ASTERORIC ») ; 9 lettres,
  même centrage. Captures de référence `title` et `controls` régénérées.

## [0.1.0] — 2026-09-15 — Sprint 1 « Portage jouable »

### Cadrage
- Portage d'Astéroric (Oric-1, `~/Oric asteroids`, 1.0.0-beta phase 40) sur
  Neo6502. Décisions PO : dépôt dédié, firmware amont mode 0, terrain
  320×240 plein écran (cf. ROADMAP.md).

### Ajouté
- Chaîne de build cc65 : `cfg/neo6502.cfg` (RAM $0800-$FBFF, pile C sous le
  noyau $FC00), `src/asm/crt0.s`, `Makefile`, `tools/mkneo.py` (format .neo).
- Couche Neo6502 : `src/asm/neo_gfx.s` (XOR via 5,1/5,2/5,5 ; lignes fermée et
  semi-ouverte sur l'EFLA du firmware), `src/neo_time.c` (5,37), `src/neo_input.c`
  (1,2 par code HID, table des noms, `key_map`), `src/neo_sound.c` (8,7 : effets
  FX_* recréés en files de notes carré/bruit sur 4 canaux, jingle Grieg).
- `src/phys.c` : intégration 8.8 avec retenue (positions `int`) et collision
  torique sur 320×240.
- Portage des modules : `game.c` (ship en C, 30 Hz, constantes recalibrées),
  `asteroids.c` (rendu polygonal arcade 11-13 sommets, wraparound par
  duplication, fragmentation), `ufo.c`, `hud.c`, `font.c`, `title.c`
  (centrage 320), `keys.c` (CONTROLS sur codes HID).
- Tables générées : `src/ship_verts.c` (apex 6 px), `src/shapes.c` (échelles
  1,1/1,9/3,1 → rayons 6/10/16).
- Tests host : `tests/host/test_phys.c`, `test_shapes.c` (stubs EFLA :
  idempotence XOR, sommets, wraparound, fragmentation), `test_keys.c`.
- Tests cible : `tests/run.sh` (Phosphoneo : captures de référence titre /
  partie / CONTROLS, programmes `tests/emu/` t_line (primitives), t_xor
  (idempotence des polygones), t_exit (retour NeoBASIC : PRINT 6*7 → 42),
  cadence sous latence co-sim, fumée `neo`),
  `tests/latency/api-latency-cosim.txt` (mesure sur le vrai firmware).
- Compteurs de diagnostic `dbg_frames` / `dbg_late` (game.c), lus par les tests.
- Documentation : README, ROADMAP, NOTICE (périmètre Atari), CLAUDE.md,
  `docs/cc65-optstackops.md`.

### Corrigé
- Sortie (ESC en game over) : `crt0.s` recharge NeoBASIC par l'API 1,3 puis
  `jmp (0)`, comme le noyau au reset — `jmp ($FFFC)` relançait le jeu dans
  les émulateurs (vecteur reset patché sur l'adresse d'exécution du .neo).
- Polygones brouillés : bug d'optimisation cc65 2.19 (`OptStackOps`
  indexait `shape_nverts` avec l'octet bas d'un pointeur) — désactivé
  globalement + lecture de `n` avant les pointeurs.
- Sommets manquants : la ligne du firmware exclut son point d'arrivée ;
  `draw_line_xor` ajoute le pixel final, `draw_line_xor_open` trace P1→P0.

### Vérifié
- `make test` : 3 tests host OK ; captures titre/partie/CONTROLS identiques
  aux références ; cadence 3 pas en retard / 255 avec la latence mesurée ;
  fumée `neo` OK. Co-sim : ligne 5,2 ≈ 544 pas ARM (≈ 22 cycles 65C02).
- Non vérifié : carte réelle (aucune disponible), rendu sonore à l'oreille.
