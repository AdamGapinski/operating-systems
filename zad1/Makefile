CC=gcc
CFLAGS=-Wall -std=gnu99
ODIR=./obj
IDIR=./include

all: Barber Client

Barber: $(ODIR)/Barber.o $(ODIR)/Semaphores.o $(ODIR)/SharedQueue.o
	$(CC) $(CFLAGS) $^ -o $@

Client: $(ODIR)/Client.o $(ODIR)/Semaphores.o $(ODIR)/SharedQueue.o
	$(CC) $(CFLAGS) $^ -o $@

$(ODIR)/Barber.o: Barber.c $(IDIR)/Common.h $(IDIR)/Semaphores.h $(IDIR)/SharedQueue.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/Client.o: Client.c $(IDIR)/Common.h $(IDIR)/Semaphores.h $(IDIR)/SharedQueue.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/SharedQueue.o: SharedQueue.c $(IDIR)/SharedQueue.h $(IDIR)/Semaphores.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/Semaphores.o: Semaphores.c $(IDIR)/Semaphores.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f Barber Client *.o $(ODIR)/*.o

.PHONY: clean
	
