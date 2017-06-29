CC=gcc
CFLAGS=-Wall -std=gnu99
ODIR=./obj

all: dir_main nftw_main

nftw_main: $(ODIR)/nftw_main.o
	$(CC) $(CFLAGS) $< -o $@

dir_main: $(ODIR)/dir_main.o
	$(CC) $(CFLAGS) $< -o $@

$(ODIR)/nftw_main.o: nftw_main.c
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/dir_main.o: dir_main.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf ./nftw_main ./dir_main *.o $(ODIR)/*.o

.PHONY: clean all
