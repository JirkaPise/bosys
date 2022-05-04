# compiler
CC = gcc

# switches for compiler
CFLAGS = -Wall -pthread -D_GNU_SOURCE
# -Wall - print all warnings
# -pthread - adds support for multithreading with the pthreads library
# -D_GNU_SOURCE - adds support for GNU extension functions

tickets_solved: tickets_solved.c
	$(CC) $(CFLAGS) tickets_solved.c -o tickets_solved
# $(CC) - gcc as compiler
# &(CFLAGS) - adds switches for compiler
# -o - sets file name

clean:
	rm tickets_solved
# rm - remove file_name