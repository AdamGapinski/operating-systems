#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <fcntl.h>
#include <unistd.h>
#include <syscall.h>
#include <signal.h>
#include "include/Utils.h"

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

char *parseTextArg(int argc, char **argv, int arg_num, char *des) {
    if (argc <= arg_num) {
        fprintf(stderr, "Error: Argument %d. - %s not specified\n", arg_num, des);
        exit(EXIT_FAILURE);
    }
    return argv[arg_num];
}

int open_file(char *path) {
    int fd;
    if ((fd = open(path, O_RDONLY)) == -1) {
        fprintf(stderr, "Error opening file: %s\n", path);
        exit(EXIT_FAILURE);
    };
    return fd;
}

long get_thread_id() {
    return syscall(SYS_gettid);
}


int parse_signal_name(char *sig) {
    if (strcmp(sig, "SIGUSR1") == 0 || strcmp(sig, "sigusr1") == 0) {
        return SIGUSR1;
    }
    if (strcmp(sig, "SIGTERM") == 0 || strcmp(sig, "sigterm") == 0) {
        return SIGTERM;
    }
    if (strcmp(sig, "SIGKILL") == 0 || strcmp(sig, "sigkill") == 0) {
        return SIGKILL;
    }
    if (strcmp(sig, "SIGSTOP") == 0 || strcmp(sig, "sigstop") == 0) {
        return SIGSTOP;
    }
    if (strcmp(sig, "SIGFPE") == 0 || strcmp(sig, "sigfpe") == 0) {
        return SIGFPE;
    }

    fprintf(stderr, "Error: Invalid signal name. Choose SIGUSR1, SIGTERM, SIGKILL or SIGSTOP\n");
    exit(EXIT_FAILURE);
}