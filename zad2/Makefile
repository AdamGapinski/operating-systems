CC=gcc
CFLAGS=-Wall -std=gnu99
LIBS=-lrt
ODIR=./obj

all: Server Client

Server: $(ODIR)/Server.o
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

Client: $(ODIR)/Client.o
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

$(ODIR)/Server.o: Server.c
	$(CC) $(CFLAGS) -c $< $(LIBS) -o $@

$(ODIR)/Client.o: Client.c
	$(CC) $(CFLAGS) -c $< $(LIBS) -o $@

clean:
	rm -f Server Client *.o $(ODIR)/*.o

.PHONY: clean
