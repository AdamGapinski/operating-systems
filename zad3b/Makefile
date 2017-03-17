CFLAGS=-std=gnu99 -Wall
LDIR=../zad1/
INCDIR=../zad1/include/
MAINDIR=../zad2/

test: mainstatic mainshared
	echo "static library results, main.c optimalization flag set:$(OPT)\n" >> results.txt
	./mainstatic >> results.txt
	echo "\nshared library results, main.c optimalization flag set:$(OPT)\n" >> results.txt
	./mainshared >> results.txt
	
mainstatic: main.o libs
	gcc main.o -o mainstatic -laddressbook -L$(LDIR) -I$(INCDIR) -static

libs:
	cd $(LDIR) && make

mainshared: main.o
	gcc main.o -o mainshared -laddressbook -L$(LDIR) -I$(INCDIR) -Wl,-rpath,$(LDIR)

main.o: $(MAINDIR)main.c $(INCDIR)linkedlistbook.h $(INCDIR)binarytreebook.h
	gcc $(CFLAGS) $(OPT) -Wall -c -fPIC $(MAINDIR)main.c

clean:
	rm -f *.o results.txt mainstatic mainshared maindynamic

.PHONY: test clean libs
