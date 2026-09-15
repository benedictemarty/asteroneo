# Makefile — AsteroNeo : portage Neo6502 d'Astéroric (cc65 + ca65)
#
# Cibles :
#   make            build/asteroneo.bin (brut, $0800) + build/asteroneo.neo
#   make run        lance dans l'émulateur officiel neo (SDL)
#   make run-phos   lance dans Phosphoneo (headless, capture PPM)
#   make test       tests host + capture headless et comparaison aux références
#   make ref        régénère les captures de référence
#   make gen        régénère src/ship_verts.c et src/shapes.c
#   make clean

CC65_HOME ?= /usr/share/cc65
NEO_FW    ?= $(HOME)/Neo6502firmware
PHOSPHONEO ?= $(HOME)/Phosphoneo/build/phosphoneo
NEO_EMU   ?= $(NEO_FW)/bin/neo

CC      = cl65
# --disable-opt OptStackOps : cc65 2.19 génère un index faux (registre A supposé
# inchangé après un calcul de pointeur) — cf. docs/cc65-optstackops.md
CFLAGS  = -t none -O --cpu 65c02 --static-locals -Wc --disable-opt,OptStackOps -I src
AFLAGS  = --cpu 65c02
CFG     = cfg/neo6502.cfg

BUILD   = build
BIN     = $(BUILD)/asteroneo.bin
NEO     = $(BUILD)/asteroneo.neo
MAP     = $(BUILD)/asteroneo.map

CSRC    = src/main.c src/game.c src/asteroids.c src/ufo.c src/hud.c \
          src/title.c src/font.c src/keys.c src/phys.c \
          src/neo_time.c src/neo_input.c src/neo_sound.c \
          src/shapes.c src/ship_verts.c
ASRC    = src/asm/crt0.s src/asm/neo_gfx.s

OBJ     = $(patsubst src/%.c,$(BUILD)/%.o,$(CSRC)) \
          $(patsubst src/asm/%.s,$(BUILD)/%.o,$(ASRC))

.PHONY: all run run-phos test host-test emu-test ref gen clean

all: $(NEO)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/%.o: src/asm/%.s | $(BUILD)
	ca65 $(AFLAGS) -I $(CC65_HOME)/asminc -o $@ $<

$(BIN): $(OBJ) $(CFG)
	ld65 -C $(CFG) -m $(MAP) -Ln $(BUILD)/asteroneo.lbl -o $@ $(OBJ) none.lib

$(NEO): $(BIN)
	python3 tools/mkneo.py $(BIN) $(NEO) 0800 0800 "AsteroNeo"

run: $(NEO)
	$(NEO_EMU) $(BIN)@800 cold keys

run-phos: $(NEO)
	$(PHOSPHONEO) $(NEO) --cycles 60000000 --screenshot tests/out/phos.ppm

gen:
	python3 tools/gen_ship.py > src/ship_verts.c
	python3 tools/gen_shapes.py > src/shapes.c

test: host-test emu-test

host-test:
	$(MAKE) -C tests/host

# Programmes de test cible (tests/emu/*.c) liés avec la couche Neo6502.
TEST_NEO = $(BUILD)/t_exit.neo $(BUILD)/t_line.neo $(BUILD)/t_xor.neo

$(BUILD)/t_exit.bin: tests/emu/t_exit.c $(BUILD)/crt0.o $(BUILD)/neo_gfx.o
	$(CC) $(CFLAGS) -C $(CFG) -o $@ $< $(BUILD)/crt0.o $(BUILD)/neo_gfx.o none.lib

$(BUILD)/t_line.bin: tests/emu/t_line.c $(BUILD)/crt0.o $(BUILD)/neo_gfx.o $(BUILD)/neo_time.o $(BUILD)/neo_sound.o
	$(CC) $(CFLAGS) -C $(CFG) -o $@ $< $(BUILD)/crt0.o $(BUILD)/neo_gfx.o $(BUILD)/neo_time.o $(BUILD)/neo_sound.o none.lib

$(BUILD)/t_xor.bin: tests/emu/t_xor.c $(BUILD)/crt0.o $(BUILD)/neo_gfx.o $(BUILD)/neo_time.o $(BUILD)/neo_sound.o $(BUILD)/asteroids.o $(BUILD)/phys.o $(BUILD)/shapes.o
	$(CC) $(CFLAGS) -C $(CFG) -o $@ $< $(BUILD)/crt0.o $(BUILD)/neo_gfx.o $(BUILD)/neo_time.o $(BUILD)/neo_sound.o $(BUILD)/asteroids.o $(BUILD)/phys.o $(BUILD)/shapes.o none.lib

$(BUILD)/t_%.neo: $(BUILD)/t_%.bin
	python3 tools/mkneo.py $< $@ 0800 0800 "$*" >/dev/null

emu-test: $(NEO) $(TEST_NEO)
	tests/run.sh check

ref: $(NEO) $(TEST_NEO)
	tests/run.sh ref

clean:
	rm -rf $(BUILD) tests/out/*
	$(MAKE) -C tests/host clean
