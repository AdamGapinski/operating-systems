CC=gcc
CFLAGS=-Wall -std=gnu99
ODIR=./obj
IDIR=./include
LDLIBS=-lpthread

all: server client

server: $(ODIR)/server.o $(ODIR)/utils.o $(ODIR)/queue.o
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

client: $(ODIR)/client.o $(ODIR)/utils.o
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

$(ODIR)/server.o: server.c $(IDIR)/utils.h $(IDIR)/queue.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/client.o: client.c $(IDIR)/utils.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/utils.o: utils.c $(IDIR)/utils.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/queue.o: queue.c $(IDIR)/queue.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f server client *.o $(ODIR)/*.o

.PHONY: clean
