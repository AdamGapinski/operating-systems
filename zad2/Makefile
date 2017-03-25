CC=gcc
CFLAGS=-Wall
IDIR=./include
ODIR=./obj

DEPS=$(IDIR)/scanner.h
OBJ=$(ODIR)/main.o $(ODIR)/scanner.o

main: $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

resource: resource.c
	$(CC) $(CFLAGS) $^ -o $@

$(ODIR)/main.o: main.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@ -I$(IDIR)

$(ODIR)/scanner.o: scanner.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@ -I$(IDIR)

clean:
	rm -f main resource *.o $(ODIR)/*.o

.PHONY: clean

