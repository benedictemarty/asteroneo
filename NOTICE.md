# NOTICE — périmètre de la licence et droits tiers

## Œuvre originale (sous licence EUPL v1.2)

Copyright © 2026 Bénédicte Marty (bmarty@mailo.com)

Le code original de ce projet est publié sous la **Licence Publique de
l'Union européenne v1.2 (EUPL-1.2)** — voir le fichier [LICENSE](LICENSE).
Texte officiel dans toutes les langues de l'UE :
<https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12>

Sont couverts par cette licence, notamment :

- la couche Neo6502 : primitives XOR sur l'API graphique
  (`src/asm/neo_gfx.s`), cadence (`src/neo_time.c`), clavier HID
  (`src/neo_input.c`) ;
- la **recréation originale** des effets sonores et du jingle sur le
  générateur du Neo6502 (`src/neo_sound.c`, `src/sound.h`) — l'arcade
  d'origine utilisait des oscillateurs analogiques discrets, aucun code
  ni donnée audio d'Atari n'est réutilisé ;
- l'adaptation de la boucle de jeu, le HUD 7 segments, l'écran titre,
  les high scores, l'écran de configuration des touches (`src/*.c`),
  héritée du projet Astéroric (Oric-1) ;
- le système de build, les tests et la documentation.

## Droits tiers — Atari (non couverts par la licence)

Ce projet est un **clone d'étude et de préservation** d'*Asteroids*
(© 1979 Atari, Inc. ; droits actuels : Atari Interactive, Inc.).

- La **logique de jeu** (machine à états, physique, fragmentation,
  IA de la soucoupe, générateur pseudo-aléatoire, barème de score) est
  **adaptée du désassemblage de la ROM arcade rev 4** (source :
  6502disassembly.com). Les noms de variables d'origine sont conservés
  à des fins de traçabilité.
- Les **formes vectorielles** (astéroïdes, vaisseau, soucoupe) dérivent
  des données extraites de la ROM (travaux de Nick Mikstas).

**Aucun droit n'est revendiqué** sur ces éléments dérivés de l'œuvre
d'Atari, qui ne sont pas couverts par la licence EUPL ci-dessus.
*Asteroids* est une marque d'Atari Interactive, Inc. Ce projet est
**non commercial**, fourni gratuitement, et n'est **ni affilié à, ni
approuvé par Atari**. Si vous réutilisez ce code, la réutilisation des
parties dérivées d'Atari est sous votre propre responsabilité.

## Outils et références

- Émulateurs de développement : Phosphoneo (EUPL-1.2, bmarty) et
  l'émulateur officiel `neo` du firmware Neo6502 (MIT, Paul Robson et
  contributeurs — <https://github.com/paulscottrobson/neo6502-firmware>).
- Chaîne de compilation : cc65 (zlib).
- Références techniques : 6502disassembly.com, Computer Archeology,
  travaux de Nick Mikstas et Jed Margolin, documentation de l'API
  Neo6502 (`api-listing.md` du firmware).
- Projet d'origine : [Astéroric](https://github.com/benedictemarty/oric-asteroids)
  (Oric-1 48 Ko, EUPL-1.2), dont ce portage reprend la logique de jeu.
