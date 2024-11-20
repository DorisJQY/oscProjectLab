# Create your own targets that compile the application

all:
	gcc -g -Wall main.c sensor_db.c -o main.out

run: all
	./main.out

memtest:
	valgrind --leak-check=yes ./main.out

