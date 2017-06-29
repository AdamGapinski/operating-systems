CC=gcc
CFLAGS=-Wall -std=gnu99
ODIR=./obj
IDIR=./include

all:
	rm -f wyniki.txt
	make record SIZE=4
	make record SIZE=512
	make record SIZE=4096
	make record SIZE=8192

record:
	make run RECORDS1=1000 RECORDS2=10000 SIZE=$(SIZE)
	make run RECORDS1=1800 RECORDS2=15000 SIZE=$(SIZE)

run: main
	rm -f ./datafile ./datacopy
	./main generate datafile $(RECORDS1) $(SIZE)
	cp ./datafile ./datacopy 
	printf "\t\tTest case for $(SIZE) bytes per each record.\n" >> wyniki.txt
	printf "\tsorting $(RECORDS1):\n\n" >> wyniki.txt
	./main lib sort datafile $(RECORDS1) $(SIZE) >> wyniki.txt
	./main sys sort datacopy $(RECORDS1) $(SIZE) >> wyniki.txt
	rm -f ./datafile ./datacopy
	./main generate datafile $(RECORDS2) $(SIZE)
	cp ./datafile ./datacopy
	printf "\tshuffling $(RECORDS2):\n\n" >> wyniki.txt
	./main lib shuffle datafile $(RECORDS2) $(SIZE) >> wyniki.txt
	./main sys shuffle datacopy $(RECORDS2) $(SIZE) >> wyniki.txt
	printf "\n\n\n" >> wyniki.txt

main: $(ODIR)/main.o $(ODIR)/time_msrmnt.o
	$(CC) $(CFLAGS) $^ -o $@

$(ODIR)/main.o: main.c $(IDIR)/time_msrmnt.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/time_msrmnt.o: time_msrmnt.c $(IDIR)/time_msrmnt.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf ./datafile ./datacopy ./main *.o $(ODIR)/*.o

.PHONY: clean all record run
