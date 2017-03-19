CC=gcc
CFLAGS=-Wall
LDIR=../zad1
IDIR=../zad1/include
MAIN_IDIR=../zad2/include
MAINDIR=../zad2
ODIR=./obj

all:	
	make clean -C ../zad1
	make clean ; rm -f results.txt
	make runopt OPT=-O0
	make runopt OPT=-O1
	make runopt OPT=-Os
	make runopt OPT=-O2
	make runopt OPT=-O3
	make runopt OPT=-Ofast

runopt: mainstatic mainshared maindynamic
	printf "Optimization flag $(OPT):\n" >> results.txt
	printf "static library results:\n" >> results.txt
	./mainstatic >> results.txt
	printf "\n\nshared library results:\n" >> results.txt
	./mainshared >> results.txt
	printf "\n\ndynamically loaded library results:\n" >> results.txt
	./maindynamic >> results.txt
	printf "\n\n\n" >> results.txt
	make clean
	make clean -C ../zad1

mainstatic: $(ODIR)/main.o $(ODIR)/t_measurement.o $(LDIR)/libaddressbookstatic.a
	$(CC) $(CFLAGS) $(ODIR)/main.o $(ODIR)/t_measurement.o -o $@ -laddressbookstatic -L$(LDIR)/

mainshared: $(ODIR)/main.o $(ODIR)/t_measurement.o $(LDIR)/libaddressbook.so
	$(CC) $(CFLAGS) $(ODIR)/main.o $(ODIR)/t_measurement.o -o $@ -laddressbook -L$(LDIR)/ -Wl,-rpath,$(LDIR)/

maindynamic: $(ODIR)/main_dynamic.o $(ODIR)/t_measurement.o $(LDIR)/libaddressbook.so
	$(CC) $(CFLAGS) $(ODIR)/main_dynamic.o $(ODIR)/t_measurement.o -o $@ -ldl

$(LDIR)/libaddressbookstatic.a:
	make OPT=$(OPT) -C ../zad1 clean libaddressbookstatic.a

$(LDIR)/libaddressbook.so:
	make OPT=$(OPT) -C ../zad1 clean libaddressbook.so

$(ODIR)/main.o: $(MAINDIR)/main.c $(IDIR)/linkedlistbook.h $(IDIR)/binarytreebook.h $(MAIN_IDIR)/t_measurement.h
	$(CC) $(CFLAGS) $(OPT) -c -fPIC $< -o $@

$(ODIR)/main_dynamic.o: $(MAINDIR)/main_dynamic.c $(IDIR)/structures.h $(MAIN_IDIR)/t_measurement.h
	$(CC) $(CFLAGS) $(OPT) -c -fPIC $< -o $@

$(ODIR)/t_measurement.o: $(MAINDIR)/t_measurement.c $(MAIN_IDIR)/t_measurement.h
	$(CC) $(CFLAGS) $(OPT) -c -fPIC $< -o $@

clean:
	rm -rf *.o $(ODIR)/*.o mainstatic mainshared maindynamic

.PHONY: clean all runopt
