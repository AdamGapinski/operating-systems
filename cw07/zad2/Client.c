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

void wait_for_signal() ;

void sigrtmin_handler(int signo) ;

ClientsQueue *clientsQueue;
int shaved = 0;
sigset_t rtmin_suspend;
int *first_client_id;

int main(int argc, char *argv[]) {
    int clients_count = parseUIntArgument(argc, argv, 1, "number of clients");
    int shaving_count = parseUIntArgument(argc, argv, 2, "number of shavings");
    init();
    start_clients(clients_count, shaving_count);
    while (waitpid(-1, NULL, 0) != -1);
    exit(EXIT_SUCCESS);
}

void sigrtmin_handler(int signo) {
    shaved = 1;
    return;
}

void init() {
    struct sigaction action;
    action.sa_handler = sigrtmin_handler;
    sigaction(SIGRTMIN, &action, NULL);

    sigfillset(&rtmin_suspend);
    sigdelset(&rtmin_suspend, SIGRTMIN);

    initSemaphores(NAME);
    clientsQueue = initQueue(NAME, 0);
    char name[strlen(NAME) + 1];
    sprintf(name, "%s1", NAME);
    first_client_id = (int *) get_shared_address(sizeof(*first_client_id), name);
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
            *first_client_id = getpid();
            release_semaphore(CLIENT_READY);
            wait_semaphore(DONE_LOCK);
            log_info("Klient %d opuszcza zaklad po zakonczeniu strzyzenia", getpid());
            release_semaphore(CLIENT_LEFT);
            --shaving_count;
        } else {
            if (enqueue(clientsQueue, getpid()) != -1) {
                log_info("Klient %d zajal miejsce w poczekalni", getpid());
                release_semaphore(CHECKING_QUEUE);
                if (shaved == 0) {
                    sigsuspend(&rtmin_suspend);
                }
                shaved = 0;
                log_info("Klient %d opuszcza zaklad po zakonczeniu strzyzenia", getpid());
                release_semaphore(CLIENT_LEFT);
                --shaving_count;
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
