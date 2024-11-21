all: main.c sensor_db.h sensor_db.c logger.h logger.c
	gcc -g -Wall main.c sensor_db.c logger.c -o main.out

run: all
	./main.out

zip:
	zip milestone2.zip *.c *.h Makefile

memtest: all
	valgrind --leak-check=yes ./main.out

clean:
	rm -r main.out
