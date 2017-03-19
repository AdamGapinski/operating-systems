CC=gcc
CFLAGS=-Wall -std=gnu99
IDIR=./include
ODIR=./obj
all: libaddressbook.so libaddressbookstatic.a

libaddressbook.so: $(ODIR)/binarytreebook.o $(ODIR)/linkedlistbook.o $(ODIR)/contactStr.o
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $^

libaddressbookstatic.a: $(ODIR)/binarytreebook.o $(ODIR)/linkedlistbook.o $(ODIR)/contactStr.o
	ar cr -o $@ $^

$(ODIR)/binarytreebook.o: binarytreebook.c $(IDIR)/binarytreebook.h $(IDIR)/contactStr.h
	$(CC) $(CFLAGS) $(OPT) -fPIC -c binarytreebook.c -o $@

$(ODIR)/linkedlistbook.o: linkedlistbook.c $(IDIR)/linkedlistbook.h $(IDIR)/contactStr.h
	$(CC) $(CFLAGS) $(OPT) -fPIC -c linkedlistbook.c -o $@

$(ODIR)/contactStr.o: contactStr.c $(IDIR)/contactStr.h
	$(CC) $(CFLAGS) $(OPT) -fPIC -c contactStr.c -o $@

clean:
	rm -rf *.so *.a $(ODIR)/*.o

.PHONY: clean all
