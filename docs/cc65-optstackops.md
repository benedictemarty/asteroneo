# cc65 2.19 : bug de l'optimisation `OptStackOps`

**Symptôme** (2026-09-15, sprint 1) : les astéroïdes s'affichent comme un
enchevêtrement de segments (« polygones brouillés »), alors que la même
logique compilée avec gcc sur l'hôte (`tests/host/test_shapes`) est correcte.

**Cause** : dans `asteroid_poly_at` (`src/asteroids.c`), cc65 V2.19
(`Git 6efe447d1`, `-O`) génère pour

```c
const signed char *vx = shape_vx[id];
const signed char *vy = shape_vy[id];
unsigned char n = shape_nverts[id];
```

la séquence

```asm
    adc  #<(_shape_vy)
    tay
    txa
    adc  #>(_shape_vy)
    sta  M0002+1
    tya
    sta  M0002
    tay                 ; Y = octet bas du pointeur…
    lda  _shape_nverts,y ; …indexe shape_nverts avec ce pointeur, pas avec id
```

L'optimiseur (`OptStackOps`, fusion des opérations sur la pile C) considère
que A contient encore `id` après le calcul `shape_vy + id*14`. Le nombre de
sommets lu est aléatoire → boucle sur des sommets hors table.

**Vérification** : `cc65 --disable-opt OptStackOps` produit le code attendu
(`ldy #$04 ; lda (c_sp),y ; tay ; lda _shape_nverts,y`) ; aucune autre
passe désactivée ne corrige.

**Correctif appliqué** :

1. `Makefile` : `-Wc --disable-opt,OptStackOps` sur tout le projet (coût :
   quelques cycles par appel de fonction, sans effet à 6,25 MHz) ;
2. `asteroid_poly_at` : `n` est lu avant les pointeurs (défense en
   profondeur, commentée dans le code).

**Leçon** : toute nouvelle passe d'optimisation activée doit être validée
par `make test` (captures de référence bit-à-bit).
