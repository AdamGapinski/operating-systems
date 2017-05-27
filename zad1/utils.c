#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syscall.h>
#include <signal.h>
#include <pthread.h>
#include "include/utils.h"

pthread_mutex_t logger_read_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t logger_write_mutex = PTHREAD_MUTEX_INITIALIZER;

char *filename = "logs";
char *logMsg = NULL;
pthread_t loggerThread;
int logThreadStarted = 0;

void createLoggerThread();

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

void setSigIntHandler(void (*handler)(int)) {
    struct sigaction sigactionStr;
    sigactionStr.sa_handler = handler;

    if (sigaction(SIGINT, &sigactionStr, NULL) == -1) {
        perror("Setting SIGINT handler error");
        exit(EXIT_FAILURE);
    }
}

long get_thread_id() {
    return syscall(SYS_gettid);
}

void *startLoggerThread(void *filename) {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    FILE *logFile;
    if ((logFile = fopen(filename, "a")) == NULL) {
        perror("logger: open file error");
        pthread_exit((void *) 1);
    }
    while (1) {
        pthread_mutex_lock(&logger_read_mutex);
        size_t info_size = sizeof(*logMsg);
        size_t info_length = strlen(logMsg);
        if (fwrite(logMsg, info_size, info_length, logFile) != info_size * info_length) {
            perror("logger: write file error");
            pthread_mutex_unlock(&logger_read_mutex);
            pthread_exit((void *) 1);
        };
        fflush(logFile);
        free(logMsg);
        pthread_mutex_unlock(&logger_write_mutex);
    }
}

void make_log(char *logArg, int var) {
    pthread_mutex_lock(&logger_write_mutex);
    if (logThreadStarted == 0) {
        pthread_mutex_lock(&logger_read_mutex);
        createLoggerThread();
    }
    if (logThreadStarted == 1) {
        logMsg = calloc(strlen(logArg) + 512, sizeof(*logMsg));
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        sprintf(logMsg, "%ld TID %ld s %ld us: %s\n", get_thread_id(), ts.tv_sec, ts.tv_nsec / 1000, logArg);
        sprintf(logMsg, logMsg, var);
    }
    pthread_mutex_unlock(&logger_read_mutex);
}

void createLoggerThread() {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    int errno;
    if ((errno = pthread_create(&loggerThread, &attr, startLoggerThread, filename)) != 0) {
        char errMsgBuf[256];
        sprintf(errMsgBuf, "Error creating logger thread: %s", strerror(errno));
        fprintf(stderr, "%s", errMsgBuf);
    } else {
        logThreadStarted = 1;
    }
    pthread_attr_destroy(&attr);
}
