CFLAGS = -std=c11 -Wall -Wextra -pedantic

zephyros: zephyros.c
	gcc $(CFLAGS) $< -o $@
