CC=gcc
CFLAGS=-Wall -std=gnu99
ODIR=./obj

all: Slave Master

Master: $(ODIR)/Master.o
	$(CC) $(CFLAGS) $^ -lm -o $@

Slave: $(ODIR)/Slave.o
	$(CC) $(CFLAGS) $^ -lm -o $@

$(ODIR)/Slave.o: Slave.c
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/Master.o: Master.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f Slave Master data *.o $(ODIR)/*.o

.PHONY: clean
