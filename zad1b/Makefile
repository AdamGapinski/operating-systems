CC=gcc
CFLAGS=-Wall -std=gnu99
ODIR=./obj
IDIR=./include
LDLIBS=-lpthread

ReaderWriterFifo: $(ODIR)/ReaderWriterFifo.o $(ODIR)/Utils.o $(ODIR)/Queue.o
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

$(ODIR)/ReaderWriterFifo.o: ReaderWriterFifo.c $(IDIR)/Utils.h $(IDIR)/Queue.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/Utils.o: Utils.c $(IDIR)/Utils.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/Queue.o: Queue.c $(IDIR)/Queue.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f ReaderWriterFifo *.o $(ODIR)/*.o

.PHONY: clean
	
