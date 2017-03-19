CC=gcc
CFLAGS=-Wall
LDIR=../zad1
IDIR=../zad1/include
MAIN_IDIR=../zad2/include
MAINDIR=../zad2
ODIR=./obj

main: $(ODIR)/main.o $(ODIR)/measurement_utils.o $(ODIR)/t_measurement.o $(LDIR)/libaddressbookstatic.a
	$(CC) $(CFLAGS) $(ODIR)/main.o $(ODIR)/measurement_utils.o $(ODIR)/t_measurement.o -o $@ -laddressbookstatic -L$(LDIR)/ -I$(IDIR)/

$(LDIR)/libaddressbookstatic.a:
	make -C ../zad1 libaddressbookstatic.a

$(ODIR)/main.o: $(MAINDIR)/main_core.c $(IDIR)/linkedlistbook.h $(IDIR)/binarytreebook.h $(MAIN_IDIR)/measurement_utils.h $(MAIN_IDIR)/t_measurement.h
	$(CC) $(CFLAGS) -c -g -fPIC $< -o $@

$(ODIR)/measurement_utils.o: $(MAINDIR)/measurement_utils.c $(MAIN_IDIR)/measurement_utils.h
	$(CC) $(CFLAGS) -c -g -fPIC $< -o $@

$(ODIR)/t_measurement.o: $(MAINDIR)/t_measurement.c $(MAIN_IDIR)/t_measurement.h
	$(CC) $(CFLAGS) -c -g -fPIC $< -o $@

clean:
	rm -rf *.o $(ODIR)/*.o main

.PHONY: clean
