CFLAGS=-std=gnu99 -Wall
INCDIR=./include/
all: libaddressbook.so libaddressbook.a

libaddressbook.so: binarytreebook.o linkedlistbook.o contactStr.o
	gcc $(CFLAGS) -shared -fPIC -o libaddressbook.so binarytreebook.o linkedlistbook.o contactStr.o

libaddressbook.a: binarytreebook.o linkedlistbook.o contactStr.o
	ar cr -o libaddressbook.a binarytreebook.o linkedlistbook.o contactStr.o

binarytreebook.o: binarytreebook.c $(INCDIR)binarytreebook.h
	gcc $(CFLAGS) -fPIC -c binarytreebook.c

linkedlistbook.o: linkedlistbook.c $(INCDIR)linkedlistbook.h
	gcc $(CFLAGS) -fPIC -c linkedlistbook.c

contactStr.o: contactStr.c $(INCDIR)contactStr.h
	gcc $(CFLAGS) -fPIC -c contactStr.c

clean:
	rm -f *.o *.so *.a

.PHONY: clean
