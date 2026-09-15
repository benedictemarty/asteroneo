;=================================================================
; crt0.s — démarrage cc65 pour le Neo6502 (AsteroNeo)
;
; Chargé à $0800, entré par le vecteur reset ($FFFC → $0800, option
; « cold » de l'émulateur) ou par l'adresse d'exécution de l'en-tête .neo.
; Initialise la pile, la pile C (sp), BSS/DATA, appelle main(), puis
; recharge NeoBASIC (API 1,3) et lui rend la main.
;=================================================================

        .export   _init, _exit
        .export   __STARTUP__ : absolute = 1
        .import   _main
        .import   __RAM_START__, __RAM_SIZE__, __STACKSIZE__
        .import   copydata, zerobss, initlib, donelib

        .include  "zeropage.inc"

        .segment  "STARTUP"

_init:
        ldx  #$FF
        txs
        cld

        lda  #<(__RAM_START__ + __RAM_SIZE__ + __STACKSIZE__)
        ldx  #>(__RAM_START__ + __RAM_SIZE__ + __STACKSIZE__)
        sta  c_sp
        stx  c_sp+1

        jsr  zerobss
        jsr  copydata
        jsr  initlib

        jsr  _main

_exit:
        jsr  donelib
        ; Retour à NeoBASIC comme le fait le noyau au reset : 1,3 « Load
        ; BASIC » recharge l'interpréteur à $0800 (par-dessus ce programme)
        ; et place son adresse de départ en $0000, puis jmp (0). Un
        ; jmp ($FFFC) relancerait le jeu dans les émulateurs (vecteur reset
        ; patché sur l'adresse d'exécution du .neo).
        sei
        ldx  #$FF
        txs
@w:     lda  $FF00
        bne  @w
        lda  #3
        sta  $FF01
        lda  #1
        sta  $FF00
@w2:    lda  $FF00
        bne  @w2
        jmp  ($0000)
