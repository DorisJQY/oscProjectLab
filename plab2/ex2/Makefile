all:
	gcc -g -Wall reverse.c -o reverse

run: all
	./reverse

memtest:
	valgrind --leak-check=yes ./reverse
