zip:
	zip milestone3.zip *.c *.h Makefile

all: main.c sbuffer.h sbuffer.c
	gcc -g -Wall main.c sbuffer.c -o buffer.out

clean:
	rm -r buffer.out

run: all
	./buffer.out
