CC=gcc
SOURCES=main.c
OBJ=./obj
IDIR=./include

main: $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJ): $(SOURCES)
	$(CC) $(CFLAGS) $^ -o $@
