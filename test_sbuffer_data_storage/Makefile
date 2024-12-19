zip:
	zip milestone3.zip *.c *.h Makefile

all: main.c sbuffer.h sbuffer.c datamgr.h datamgr.c sensor_db.h sensor_db.c lib/dplist.h lib/dplist.c
	gcc -g -Wall main.c sbuffer.c datamgr.c sensor_db.c lib/dplist.c -o buffer.out

clean:
	rm -r buffer.out

run: all
	./buffer.out
