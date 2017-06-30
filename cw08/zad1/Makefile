CC=gcc
CFLAGS=-Wall -std=gnu99
ODIR=./obj
IDIR=./include
LDLIBS=-lpthread

all: FinderAsync FinderSync FinderDetached

FinderAsync: $(ODIR)/FinderAsync.o $(ODIR)/Utils.o
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

FinderSync: $(ODIR)/FinderSync.o $(ODIR)/Utils.o
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

FinderDetached: $(ODIR)/FinderDetached.o $(ODIR)/Utils.o
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

$(ODIR)/FinderAsync.o: FinderAsync.c $(IDIR)/Utils.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/FinderSync.o: FinderSync.c $(IDIR)/Utils.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/FinderDetached.o: FinderDetached.c $(IDIR)/Utils.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/Utils.o: Utils.c $(IDIR)/Utils.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f FinderAsync FinderSync FinderDetached *.o $(ODIR)/*.o

.PHONY: clean
	
