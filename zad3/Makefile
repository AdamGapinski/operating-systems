CC=gcc
CFLAGS=-Wall -std=gnu99
ODIR=./obj

main: $(ODIR)/main.o
	$(CC) $(CFLAGS) $< -o $@

$(ODIR)/main.o: main.c
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -rf ./main *.o $(ODIR)/*.o

.PHONY: clean
