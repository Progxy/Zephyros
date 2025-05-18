CFLAGS = -std=c11 -Wall -Wextra -pedantic -ggdb -lm

all: zrest zrest-win

zrest: zrest.c ../zephyros.h
	gcc $(CFLAGS) $< -o $@

zrest-win: zrest.c ../zephyros.h
	x86_64-w64-mingw32-gcc $(CFLAGS) $< -o $@.exe -lws2_32

