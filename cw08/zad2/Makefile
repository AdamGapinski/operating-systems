CC=gcc
CFLAGS=-Wall -Wno-div-by-zero -std=gnu99
ODIR=./obj
IDIR=./include
LDLIBS=-lpthread

Main: $(ODIR)/Main.o $(ODIR)/Utils.o
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

$(ODIR)/Main.o: Main.c $(IDIR)/Utils.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/Utils.o: Utils.c $(IDIR)/Utils.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f Main *.o $(ODIR)/*.o

.PHONY: clean
