#include <stdio.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <unistd.h>
#include <wait.h>
#include <time.h>
#include <wchar.h>
#include <stddef.h>
#include "include/Common.h"
#include "include/Semaphores.h"
#include "include/SharedQueue.h"

int parseUIntArgument(int argc, char **argv, int arg_num, char *des);

void start_client(int i);

void start_clients(int clients_count, int shaving_count);

void log_info(char *info, int var) ;

void init();

ClientsQueue *clientsQueue;

int main(int argc, char *argv[]) {
    int clients_count = parseUIntArgument(argc, argv, 1, "number of clients");
    int shaving_count = parseUIntArgument(argc, argv, 2, "number of shavings");
    init();
    start_clients(clients_count, shaving_count);
    while (waitpid(-1, NULL, 0) != -1);
    exit(EXIT_SUCCESS);
}

void init() {
    initSemaphores(PATHNAME);
    clientsQueue = initQueue(PATHNAME, CLIENTS_QUEUE_KEY, 0);
}

void start_clients(int clients_count, int shaving_count) {
    for (int i = 0; i < clients_count; ++i) {
        int pid;
        if ((pid = fork()) == 0) {
            start_client(shaving_count);
            exit(EXIT_SUCCESS);
        } else if (pid == -1) {
            exit(EXIT_FAILURE);
        };
    }
}

void start_client(int shaving_count) {
    while (shaving_count > 0) {
        wait_semaphore(CHECKING_QUEUE);
        if (nowait_semaphore(BARBER_FREE_TO_WAKE_UP) == 1) {
            release_semaphore(CHECKING_QUEUE);
            log_info("Golibroda obudzony przez %d", getpid());
            release_semaphore(CLIENT_READY);
            wait_semaphore(DONE_LOCK);
            log_info("Klient %d opuszcza zaklad po zakonczeniu strzyzenia", getpid());
            --shaving_count;
            release_semaphore(CLIENT_LEFT);
        } else {
            if (enqueue(clientsQueue, getpid()) != -1) {
                log_info("Klient %d zajal miejsce w poczekalni", getpid());
                release_semaphore(CHECKING_QUEUE);
                while (get_semaphore(CLIENT_PID) != getpid()) {
                    wait_semaphore_acquired(CHECK_PID);
                }
                release_semaphore(CHECK_PID);
                set_semaphore(CLIENT_PID, 0);
                log_info("Klient %d opuszcza zaklad po zakonczeniu strzyzenia", getpid());
                --shaving_count;
                release_semaphore(CLIENT_LEFT);
            } else {
                release_semaphore(CHECKING_QUEUE);
                log_info("Klient %d opuszcza zaklad z powodu braku wolnych miejsc w poczekalni", getpid());
                wait_semaphore(NEXT);
            }
        }
    }
}

void log_info(char *info, int var) {
    char buf[strlen(info) + 256];
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    sprintf(buf, "%ld s %ld us: %s\n", ts.tv_sec, ts.tv_nsec / 1000, info);
    printf(buf, var);
    fflush(stdout);
}

int parseUIntArgument(int argc, char **argv, int arg_num, char *des) {
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
