CFLAGS = -std=c11 -Wall -Wextra -pedantic -lm

test: zephyros_test
	./zephyros_test

zephyros_test: zephyros_test.c zephyros.h
	gcc $(CFLAGS) $< -o $@

lzephyros: zephyros.c
	gcc $(CFLAGS) -fPIC -shared $< -o lib$@.so

win_dll: zephyros.c
	gcc $(CFLAGS) -shared $< -o $@.dll

