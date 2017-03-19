CC=gcc
CFLAGS=-Wall -std=gnu99
LDIR=../zad1
IDIR=../zad1/include
MAIN_IDIR=../zad2/include
MAINDIR=../zad2
ODIR=./obj

test: mainstatic mainshared maindynamic
	printf "static library results:\n" > results.txt
	./mainstatic >> results.txt
	printf "\n\nshared library results:\n" >> results.txt
	./mainshared >> results.txt
	printf "\n\ndynamically loaded library resultst:\n" >> results.txt
	./maindynamic >> results.txt

mainstatic: $(ODIR)/main.o $(ODIR)/t_measurement.o $(LDIR)/libaddressbookstatic.a
	$(CC) $(CFLAGS) $(ODIR)/main.o $(ODIR)/t_measurement.o -o $@ -laddressbookstatic -L$(LDIR)/

mainshared: $(ODIR)/main.o $(ODIR)/t_measurement.o $(LDIR)/libaddressbook.so
	$(CC) $(CFLAGS) $(ODIR)/main.o $(ODIR)/t_measurement.o -o $@ -laddressbook -L$(LDIR)/ -Wl,-rpath,$(LDIR)/

maindynamic: $(ODIR)/main_dynamic.o $(ODIR)/t_measurement.o $(LDIR)/libaddressbook.so
	$(CC) $(CFLAGS) $(ODIR)/main_dynamic.o $(ODIR)/t_measurement.o -o $@ -ldl

$(LDIR)/libaddressbookstatic.a:
	make -C $(LDIR) libaddressbookstatic.a

$(LDIR)/libaddressbook.so:
	make -C $(LDIR) libaddressbook.so

$(ODIR)/main.o: $(MAINDIR)/main.c $(IDIR)/linkedlistbook.h $(IDIR)/binarytreebook.h $(MAIN_IDIR)/t_measurement.h
	$(CC) $(CFLAGS) -c -fPIC $< -o $@

$(ODIR)/main_dynamic.o: $(MAINDIR)/main_dynamic.c $(IDIR)/structures.h $(MAIN_IDIR)/t_measurement.h
	$(CC) $(CFLAGS) -c -fPIC $< -o $@

$(ODIR)/t_measurement.o: $(MAINDIR)/t_measurement.c $(MAIN_IDIR)/t_measurement.h
	$(CC) $(CFLAGS) -c -fPIC $< -o $@

clean:
	rm -rf *.o $(ODIR)/*.o mainstatic mainshared maindynamic

.PHONY: test clean
