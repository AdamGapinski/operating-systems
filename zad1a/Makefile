CC=gcc
CFLAGS=-Wall -std=gnu99
ODIR=./obj
IDIR=./include
LDLIBS=-lpthread

ReaderWriterSemaphores: $(ODIR)/ReaderWriterSemaphores.o $(ODIR)/Utils.o
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

$(ODIR)/ReaderWriterSemaphores.o: ReaderWriterSemaphores.c $(IDIR)/Utils.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/Utils.o: Utils.c $(IDIR)/Utils.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f ReaderWriterSemaphores *.o $(ODIR)/*.o

.PHONY: clean
	
