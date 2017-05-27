#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syscall.h>
#include "include/utils.h"

char *parseTextArg(int argc, char **argv, int arg_num, char *des) {
    if (argc <= arg_num) {
        fprintf(stderr, "Error: Argument %d. - %s not specified\n", arg_num, des);
        exit(EXIT_FAILURE);
    }
    return argv[arg_num];
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

long get_thread_id() {
    return syscall(SYS_gettid);
}
