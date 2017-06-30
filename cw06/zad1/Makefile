CC=gcc
CFLAGS=-Wall -std=gnu99
ODIR=./obj

all: Server Client

Server: $(ODIR)/Server.o
	$(CC) $(CFLAGS) $^ -o $@

Client: $(ODIR)/Client.o
	$(CC) $(CFLAGS) $^ -o $@

$(ODIR)/Server.o: Server.c common.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/Client.o: Client.c common.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f Server Client *.o $(ODIR)/*.o

.PHONY: clean
