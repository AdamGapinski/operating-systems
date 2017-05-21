#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syscall.h>
#include "include/Utils.h"

int parseVerboseArg(int argc, char **argv) {
    int arg_offset = 0;
    if (argc < 2) {
        fprintf(stderr, "Wrong number of arguments\n");
        exit(EXIT_FAILURE);
    } else if (strcmp(argv[1], "-i") == 0) {
        verbose = 1;
        arg_offset = 1;
    }
    return arg_offset;
}

int parseUnsignedIntArg(int argc, char **argv, int arg_num, char *des) {
    if (argc <= arg_num) {
        fprintf(stderr, "Error: Argument %d. - %s not specified\n", arg_num, des);
        exit(EXIT_FAILURE);
    }
    char *endPtr;
    int result = (int) strtol(argv[arg_num], &endPtr, 10);
    if (result < 0 || strcmp(endPtr, "") != 0) {
        fprintf(stderr, "Error: Invalid argument %d. - %s\n", arg_num, des);
        exit(EXIT_FAILURE);
    }
    return result;
}

int *init_integer_array() {
    int *integers_array = malloc(sizeof(*integers_array) * INT_ARRAY_SIZE);
    for (int i = 0; i < INT_ARRAY_SIZE; ++i) {
        integers_array[i] = rand();
    }
    return integers_array;
}

long get_thread_id() {
    return syscall(SYS_gettid);
}
