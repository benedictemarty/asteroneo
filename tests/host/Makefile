# Tests host (x86) — logique portable compilée avec gcc contre des stubs de tracé
CC = gcc
CFLAGS = -w -I ../../src -I .
SRC = ../../src

all: test_phys test_shapes test_keys
	./test_phys && ./test_shapes && ./test_keys

test_phys: test_phys.c $(SRC)/phys.c
	$(CC) $(CFLAGS) -o $@ test_phys.c $(SRC)/phys.c

test_shapes: test_shapes.c stubs.c $(SRC)/asteroids.c $(SRC)/phys.c $(SRC)/shapes.c
	$(CC) $(CFLAGS) -o $@ test_shapes.c stubs.c $(SRC)/asteroids.c $(SRC)/phys.c $(SRC)/shapes.c

test_keys: test_keys.c $(SRC)/neo_input.c
	$(CC) $(CFLAGS) -DHOST_TEST -o $@ test_keys.c

clean:
	rm -f test_phys test_shapes test_keys
