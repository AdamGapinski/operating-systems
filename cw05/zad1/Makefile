CC=gcc
CFLAGS=-Wall -std=gnu99
ODIR=./obj

OBJ=$(ODIR)/main.o

all: main

main: $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

$(ODIR)/main.o: main.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f main *.o $(ODIR)/*.o

.PHONY: clean
