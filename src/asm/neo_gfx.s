;=================================================================
; neo_gfx.s — Primitives XOR sur l'API graphique du Neo6502
;
; Remplace line.s (Bresenham HIRES) de la version Oric : le tracé est
; fait par le RP2040 (fonctions 5,2 ligne et 5,5 pixel) en mode XOR
; (5,1 : And=$FF, Xor=couleur). Le 6502 ne fait que remplir les
; paramètres 16 bits et attendre la fin de la commande.
;
; Exports C : _gfx_init, _gfx_clear, _draw_line_xor,
;             _draw_line_xor_open, _plot_dot
; ZP exportés : _lx0, _ly0, _lx1, _ly1 (int)
;=================================================================

        .export   _gfx_init, _gfx_clear
        .export   _draw_line_xor, _draw_line_xor_open, _plot_dot
        .exportzp _lx0, _ly0, _lx1, _ly1

NEO_CMD  = $FF00
NEO_FN   = $FF01
NEO_P    = $FF04

G_CONSOLE  = 2
G_GRAPHICS = 5
F_CLEAR    = 12
F_DEFAULTS = 1
F_LINE     = 2
F_PIXEL    = 5

INK        = 255

        .zeropage
_lx0:   .res 2
_ly0:   .res 2
_lx1:   .res 2
_ly1:   .res 2

        .segment "CODE"

; Attend la fin de la commande en cours.
wait:
        lda  NEO_CMD
        bne  wait
        rts

; Copie lx0/ly0 dans P0..P3 (commun ligne / pixel).
set_p0:
        lda  _lx0
        sta  NEO_P+0
        lda  _lx0+1
        sta  NEO_P+1
        lda  _ly0
        sta  NEO_P+2
        lda  _ly0+1
        sta  NEO_P+3
        rts

;-----------------------------------------------------------------
; _gfx_init — efface l'écran (console 2,12) puis passe en XOR.
;-----------------------------------------------------------------
_gfx_init:
        jsr  wait
        lda  #F_CLEAR
        sta  NEO_FN
        lda  #G_CONSOLE
        sta  NEO_CMD
        jsr  wait
        lda  #$FF               ; And : conserve le pixel
        sta  NEO_P+0
        lda  #INK               ; Xor : bascule vers/depuis la couleur
        sta  NEO_P+1
        lda  #0
        sta  NEO_P+2            ; pas de remplissage
        sta  NEO_P+4            ; pas de flip
        lda  #1
        sta  NEO_P+3            ; taille 1
        lda  #F_DEFAULTS
        sta  NEO_FN
        lda  #G_GRAPHICS
        sta  NEO_CMD
        jmp  wait

;-----------------------------------------------------------------
; _gfx_clear — efface l'écran sans toucher aux réglages XOR.
;-----------------------------------------------------------------
_gfx_clear:
        jsr  wait
        lda  #F_CLEAR
        sta  NEO_FN
        lda  #G_CONSOLE
        sta  NEO_CMD
        jmp  wait

;-----------------------------------------------------------------
; _plot_dot — XOR du pixel (lx0, ly0).
;-----------------------------------------------------------------
_plot_dot:
        jsr  wait
        jsr  set_p0
        lda  #F_PIXEL
        sta  NEO_FN
        lda  #G_GRAPHICS
        sta  NEO_CMD
        jmp  wait

;-----------------------------------------------------------------
; La ligne du firmware (EFLA, efla.cpp) est SEMI-OUVERTE : elle trace
; [P0, P1[ — le point d'arrivée n'est pas peint, et un segment
; dégénéré ne trace rien. D'où :
;   _draw_line_xor      = ligne P0→P1 + pixel P1      (fermée [P0, P1])
;   _draw_line_xor_open = ligne P1→P0                 (]P0, P1] : P1 peint,
;                         P0 non — tracé dans l'autre sens)
;-----------------------------------------------------------------

; line_api — envoie la commande ligne avec P0..P7 déjà remplis.
line_api:
        lda  #F_LINE
        sta  NEO_FN
        lda  #G_GRAPHICS
        sta  NEO_CMD
        jmp  wait

;-----------------------------------------------------------------
; _draw_line_xor — ligne XOR (lx0, ly0) → (lx1, ly1), extrémités incluses.
;-----------------------------------------------------------------
_draw_line_xor:
        jsr  wait
        jsr  set_p0
        lda  _lx1
        sta  NEO_P+4
        lda  _lx1+1
        sta  NEO_P+5
        lda  _ly1
        sta  NEO_P+6
        lda  _ly1+1
        sta  NEO_P+7
        jsr  line_api
        ; pixel d'arrivée (exclu par l'EFLA)
        lda  _lx1
        sta  NEO_P+0
        lda  _lx1+1
        sta  NEO_P+1
        lda  _ly1
        sta  NEO_P+2
        lda  _ly1+1
        sta  NEO_P+3
        lda  #F_PIXEL
        sta  NEO_FN
        lda  #G_GRAPHICS
        sta  NEO_CMD
        jmp  wait

;-----------------------------------------------------------------
; _draw_line_xor_open — ](lx0,ly0), (lx1,ly1)] : ligne P1→P0 (l'EFLA
; peint son départ P1 et exclut son arrivée P0). Dégénéré : rien.
;-----------------------------------------------------------------
_draw_line_xor_open:
        jsr  wait
        lda  _lx1
        sta  NEO_P+0
        lda  _lx1+1
        sta  NEO_P+1
        lda  _ly1
        sta  NEO_P+2
        lda  _ly1+1
        sta  NEO_P+3
        lda  _lx0
        sta  NEO_P+4
        lda  _lx0+1
        sta  NEO_P+5
        lda  _ly0
        sta  NEO_P+6
        lda  _ly0+1
        sta  NEO_P+7
        jmp  line_api
