CC=gcc
CFLAGS=-Wall
LDIR=../zad1
IDIR=../zad1/include
MAIN_IDIR=../zad2/include
MAINDIR=../zad2
ODIR=./obj

test: mainstatic mainshared
	printf "static library results:\n" > results.txt
	./mainstatic >> results.txt
	printf "\n\nshared library results:\n" >> results.txt
	./mainshared >> results.txt

mainstatic: $(ODIR)/main.o $(ODIR)/measurement_utils.o $(ODIR)/t_measurement.o $(LDIR)/libaddressbookstatic.a
	$(CC) $(CFLAGS) $(ODIR)/main.o $(ODIR)/measurement_utils.o $(ODIR)/t_measurement.o -o $@ -laddressbookstatic -L$(LDIR)/ -I$(IDIR)/

mainshared: $(ODIR)/main.o $(ODIR)/measurement_utils.o $(ODIR)/t_measurement.o $(LDIR)/libaddressbook.so
	$(CC) $(CFLAGS) $(ODIR)/main.o $(ODIR)/measurement_utils.o $(ODIR)/t_measurement.o -o $@ -laddressbook -L$(LDIR)/ -I$(IDIR)/ -Wl,-rpath,$(LDIR)/

maindynamic: $(ODIR)/main.o $(ODIR)/measurement_utils.o $(ODIR)/t_measurement.o
	$(CC) $(CFLAGS) $(ODIR)/main.o $(ODIR)/measurement_utils.o $(ODIR)/t_measurement.o -ldl -o $@ 

$(LDIR)/libaddressbookstatic.a:
	make -C ../zad1 libaddressbookstatic.a

$(LDIR)/libaddressbook.so:
	make -C ../zad1 libaddressbook.so

$(ODIR)/main.o: $(MAINDIR)/main.c $(IDIR)/linkedlistbook.h $(IDIR)/binarytreebook.h $(MAIN_IDIR)/measurement_utils.h $(MAIN_IDIR)/t_measurement.h
	$(CC) $(CFLAGS) -c -fPIC $< -o $@

$(ODIR)/measurement_utils.o: $(MAINDIR)/measurement_utils.c $(MAIN_IDIR)/measurement_utils.h
	$(CC) $(CFLAGS) -c -fPIC $< -o $@

$(ODIR)/t_measurement.o: $(MAINDIR)/t_measurement.c $(MAIN_IDIR)/t_measurement.h
	$(CC) $(CFLAGS) -c -fPIC $< -o $@

clean:
	rm -rf *.o $(ODIR)/*.o mainstatic mainshared maindynamic

.PHONY: test clean
