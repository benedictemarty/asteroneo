;=================================================================
; crt0.s — démarrage cc65 pour le Neo6502 (AsteroNeo)
;
; Chargé à $0800, entré par le vecteur reset ($FFFC → $0800, option
; « cold » de l'émulateur) ou par l'adresse d'exécution de l'en-tête .neo.
; Initialise la pile, la pile C (sp), BSS/DATA, appelle main(), puis
; redémarre la machine (retour au noyau / NeoBASIC).
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
        jmp  ($FFFC)            ; reset → noyau (NeoBASIC)
