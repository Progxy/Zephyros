CFLAGS = -std=c11 -Wall -Wextra -pedantic

test: zephyros
	./zephyros

zephyros: zephyros.c zephyros.h
	gcc $(CFLAGS) $< -o $@
