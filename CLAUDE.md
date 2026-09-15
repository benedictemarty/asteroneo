# CLAUDE.md — AsteroNeo

Portage Neo6502 d'Astéroric (`~/Oric asteroids`). Lire README.md (architecture),
ROADMAP.md (sprints, décisions) et CHANGELOG.md avant toute modification.

## Règles du projet
- Méthode agile : chaque modification = code + tests + CHANGELOG + docs à jour
  (README/ROADMAP), puis commit `bmarty <bmarty@mailo.com>`, jamais de
  co-auteur IA dans les messages.
- `make test` doit passer avant tout commit (tests host gcc + captures
  Phosphoneo bit-à-bit + cadence + fumée `neo`). Un changement visuel voulu
  se valide par `make ref` après inspection des captures (`tests/out/*.ppm`).
- Cible = **firmware amont, mode 0, API standard** : ne pas utiliser les
  fonctions du fork (modes 1/2, tick IRQ, pages) sans décision du PO.
- Ne pas inventer : ce qui n'est pas mesuré (latence carte, rendu sonore)
  est indiqué comme non vérifié.

## Chaîne d'outils
- cc65 2.19 (`cl65 -t none --cpu 65c02`), **`-Wc --disable-opt,OptStackOps`
  obligatoire** (docs/cc65-optstackops.md).
- Phosphoneo (`~/Phosphoneo/build/phosphoneo`) : oracle déterministe
  (`--cycles`, `--type-keys C:TEXT`, `--screenshot-at C:FILE`,
  `--api-latency`, `--dump-ram-when A:V:FILE`, `--neo-emu ELF` pour la
  co-simulation sur le vrai firmware).
- `neo` (`~/Neo6502firmware/bin/neo`, fork bmarty avec hooks `cycles:`,
  `shot:`, `keys:`) : jeu interactif et fumée. Le firmware/API de référence
  est `~/Neo6502firmware/firmware/common/` (`bin/api-listing.md`).

## Pièges connus
- Ligne 5,2 du firmware = EFLA **semi-ouverte** `[P0, P1[`, segment
  dégénéré = rien : passer par `neo_gfx.s`, jamais par l'API en direct.
- Coordonnées en `int` : `(x << 8)` déborde 16 bits → `integrate()` (phys.c).
- `signed char` en arithmétique `int` : vérifier le code généré si un
  nouveau motif d'accès apparaît (le bug OptStackOps s'est manifesté ainsi).
- Les symboles pour les tests (`_dbg_frames`) viennent de `build/asteroneo.lbl`.
