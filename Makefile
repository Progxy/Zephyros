CFLAGS = -std=c11 -Wall -Wextra -pedantic -lm

test: zephyros
	./zephyros

zephyros: zephyros.c zephyros.h
	gcc $(CFLAGS) $< -o $@
