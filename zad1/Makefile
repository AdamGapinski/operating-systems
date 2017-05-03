CC=gcc
CFLAGS=-Wall -std=gnu99
ODIR=./obj
IDIR=./include

all: Barber Client

Barber: $(ODIR)/Barber.o
	$(CC) $(CFLAGS) $^ -o $@

Client: $(ODIR)/Client.o
	$(CC) $(CFLAGS) $^ -o $@

$(ODIR)/Barber.o: Barber.c $(IDIR)/Common.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/Client.o: Client.c $(IDIR)/Common.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f Barber Client *.o $(ODIR)/*.o

.PHONY: clean
	
