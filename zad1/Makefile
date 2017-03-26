CC=gcc
CFLAGS=-Wall
IDIR=./include
ODIR=./obj

DEPS=$(IDIR)/scanner.h
OBJ=$(ODIR)/main.o $(ODIR)/scanner.o

all: main envreader

main: $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

envreader: envreader.c
	$(CC) $(CFLAGS) $^ -o $@

$(ODIR)/main.o: main.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@ -I$(IDIR)

$(ODIR)/scanner.o: scanner.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@ -I$(IDIR)

clean:
	rm -f main envreader *.o $(ODIR)/*.o

.PHONY: clean

