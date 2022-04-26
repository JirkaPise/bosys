#kompilátor
CC = gcc

# přepínače pro kompilátor
CFLAGS = -Wall -D_REENTRANT
# -Wall - vypisovat všechna varování
# -D_REENTRANT - pro vícevláknový program

tickets_solved: tickets_solved.c
	$(CC) $(CFLAGS) tickets_solved.c -o tickets_solved
# $(CC) - gcc jako kompolátor
# &(CFLAGS) - přidá přepínače pro kompilátor
# -o - nastavení názvu výstupního souboru

clean:
	rm tickets_solved
# rm - remove název_souboru