CC=gcc
CFLAGS=-Wall
OPT=
LDIR=../zad1
IDIR=../zad1/include
MAIN_IDIR=../zad2/include
MAINDIR=../zad2
ODIR=./obj

all:	
	make clean ; rm -f results.txt
	make runopt OPT=-O0
	make runopt OPT=-O1
	make runopt OPT=-Os
	make runopt OPT=-O2
	make runopt OPT=-O3
	make runopt OPT=-Ofast

runopt: mainstatic mainshared 
	printf "Optimization flag $(OPT):\n" >> results.txt
	printf "static library results:\n" >> results.txt
	./mainstatic >> results.txt
	printf "\n\nshared library results:\n" >> results.txt
	./mainshared >> results.txt
	printf "\n\n\n" >> results.txt
	make clean

mainstatic: $(ODIR)/main.o $(ODIR)/measurement_utils.o $(ODIR)/t_measurement.o $(LDIR)/libaddressbookstatic.a
	$(CC) $(CFLAGS) $(ODIR)/main.o $(ODIR)/measurement_utils.o $(ODIR)/t_measurement.o -o $@ -laddressbookstatic -L$(LDIR)/ -I$(IDIR)/

mainshared: $(ODIR)/main.o $(ODIR)/measurement_utils.o $(ODIR)/t_measurement.o $(LDIR)/libaddressbook.so
	$(CC) $(CFLAGS) $(ODIR)/main.o $(ODIR)/measurement_utils.o $(ODIR)/t_measurement.o -o $@ -laddressbook -L$(LDIR)/ -I$(IDIR)/ -Wl,-rpath,$(LDIR)/

$(LDIR)/libaddressbookstatic.a:
	make -C ../zad1 libaddressbookstatic.a

$(LDIR)/libaddressbook.so:
	make -C ../zad1 libaddressbook.so

$(ODIR)/main.o: $(MAINDIR)/main.c $(IDIR)/linkedlistbook.h $(IDIR)/binarytreebook.h $(MAIN_IDIR)/measurement_utils.h $(MAIN_IDIR)/t_measurement.h
	$(CC) $(CFLAGS) $(OPT) -c -fPIC $< -o $@

$(ODIR)/measurement_utils.o: $(MAINDIR)/measurement_utils.c $(MAIN_IDIR)/measurement_utils.h
	$(CC) $(CFLAGS) $(OPT) -c -fPIC $< -o $@

$(ODIR)/t_measurement.o: $(MAINDIR)/t_measurement.c $(MAIN_IDIR)/t_measurement.h
	$(CC) $(CFLAGS) $(OPT) -c -fPIC $< -o $@

clean:
	rm -rf *.o $(ODIR)/*.o mainstatic mainshared maindynamic

.PHONY: clean all runopt
