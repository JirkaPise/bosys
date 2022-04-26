#kompilátor
CC = gcc

# přepínače pro kompilátor
CFLAGS = -Wall -pthread -D_GNU_SOURCE
# -Wall - vypisovat všechna varování
# -pthread -
# -D_GNU_SOURCE -

tickets_solved: tickets_solved.c
	$(CC) $(CFLAGS) tickets_solved.c -o tickets_solved
# $(CC) - gcc jako kompolátor
# &(CFLAGS) - přidá přepínače pro kompilátor
# -o - nastavení názvu výstupního souboru

clean:
	rm tickets_solved
# rm - remove název_souboru