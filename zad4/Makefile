CC=gcc
CFLAGS=-Wall -std=gnu99
LDIR=../zad1
IDIR=../zad1/include
MAIN_IDIR=../zad2/include
MAINDIR=../zad2
ODIR=./obj

main: $(ODIR)/main_core.o $(ODIR)/t_measurement.o $(LDIR)/libaddressbookstatic.a
	$(CC) $(CFLAGS) $(ODIR)/main_core.o $(ODIR)/t_measurement.o -o $@ -laddressbookstatic -L$(LDIR)/ -I$(IDIR)/

$(LDIR)/libaddressbookstatic.a:
	make -C ../zad1 libaddressbookstatic.a

$(ODIR)/main_core.o: $(MAINDIR)/main_core.c $(IDIR)/linkedlistbook.h $(IDIR)/binarytreebook.h $(IDIR)/structures.h $(MAIN_IDIR)/t_measurement.h
	$(CC) $(CFLAGS) -c -g -fPIC $< -o $@

$(ODIR)/t_measurement.o: $(MAINDIR)/t_measurement.c $(MAIN_IDIR)/t_measurement.h
	$(CC) $(CFLAGS) -c -g -fPIC $< -o $@

clean:
	rm -rf *.o $(ODIR)/*.o main

.PHONY: clean
