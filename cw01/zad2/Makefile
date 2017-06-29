CC=gcc
CFLAGS=-Wall -std=gnu99
LDIR=../zad1
IDIR=../zad1/include
ODIR=./obj

main: $(ODIR)/main.o $(ODIR)/t_measurement.o $(LDIR)/libaddressbookstatic.a
	$(CC) $(CFLAGS) $(ODIR)/main.o $(ODIR)/t_measurement.o -o $@ -laddressbookstatic -L$(LDIR)/

$(LDIR)/libaddressbookstatic.a:
	make -C $(LDIR) libaddressbookstatic.a

$(ODIR)/main.o: main.c $(IDIR)/linkedlistbook.h $(IDIR)/binarytreebook.h $(IDIR)/structures.h include/t_measurement.h
	$(CC) $(CFLAGS) -c -fPIC $< -o $@

$(ODIR)/t_measurement.o: t_measurement.c include/t_measurement.h
	$(CC) $(CFLAGS) -c -fPIC $< -o $@

clean:
	rm -rf *.o $(ODIR)/*.o main

.PHONY: clean
