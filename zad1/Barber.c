#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include "include/Common.h"

int parseClientsQueueSize(int argc, char *argv[]) ;

int main(int argc, char *argv[]) {
    int size = parseClientsQueueSize(argc, argv);
}

int parseClientsQueueSize(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error: Wrong number of arguments. Specify one argument - size of clients queue\n");
        exit(EXIT_FAILURE);
    }
    char *endPtr;
    int queueSize = (int) strtol(argv[1], &endPtr, 10);
    if (queueSize <= 0 || strcmp(endPtr, "") != 0) {
        fprintf(stderr, "Error: Invalid clients queue size argument\n");
        exit(EXIT_FAILURE);
    }
    return queueSize;
}
