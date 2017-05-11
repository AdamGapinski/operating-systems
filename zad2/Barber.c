#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "include/Common.h"
#include "include/Semaphores.h"
#include "include/SharedQueue.h"

int parseClientsQueueSize(int argc, char *argv[]) ;

void clean();

void sigintExitHandler(int sig) ;

void log_info(char *info, int id);

void init(int queue_size) ;

ClientsQueue *clientsQueue;
int *first_client_id;
sigset_t rtmin_suspend;

int main(int argc, char *argv[]) {
    int size = parseClientsQueueSize(argc, argv);
    init(size);
    log_info("Barber started as process with id %d", getpid());
    while (1) {
        wait_semaphore(CHECKING_QUEUE);
        if (clients_queue_is_empty(clientsQueue) == 1) {
            log_info("Golibroda zasypia", 0);
            release_semaphore(BARBER_FREE_TO_WAKE_UP);
            release_semaphore(CHECKING_QUEUE);
            wait_semaphore(CLIENT_READY);
            log_info("Strzyzenie klienta o identyfikatorze %d", *first_client_id);
            log_info("Zakonczenie strzyzenia klienta o identyfikatorze %d", *first_client_id);
            release_semaphore(DONE_LOCK);
            release_semaphore(NEXT);
            wait_semaphore(CLIENT_LEFT);
        } else {
            int client_id;
            if ((client_id = dequeue(clientsQueue)) == -1) {
                perror("Error while client dequeue");
                exit(EXIT_FAILURE);
            }
            release_semaphore(CHECKING_QUEUE);
            release_semaphore(NEXT);
            log_info("Strzyzenie klienta o identyfikatorze %d", client_id);
            log_info("Zakonczenie strzyzenia klienta o identyfikatorze %d", client_id);
            kill(client_id, SIGRTMIN);
            timed_wait_semaphore(CLIENT_LEFT, 1);
        }
    }
}

void init(int queue_size) {
    signal(SIGINT, sigintExitHandler);
    atexit(clean);

    removeSemaphores(NAME);
    initSemaphores(NAME);
    removeQueue(initQueue(NAME, 0), NAME);
    clientsQueue = initQueue(NAME, queue_size);

    char name[strlen(NAME) + 1];
    sprintf(name, "%s1", NAME);
    first_client_id = (int *) get_shared_address(sizeof(*first_client_id), name);

    release_semaphore(QUEUE_SYNCHRONIZATION);
    release_semaphore(CHECKING_QUEUE);
}


void sigintExitHandler(int sig) {
    exit(EXIT_SUCCESS);
}

void clean() {
    removeSemaphores(NAME);
    removeQueue(clientsQueue, NAME);
}

void log_info(char *info, int var) {
    char buf[strlen(info) + 256];
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    sprintf(buf, "%ld s %ld us: %s\n", ts.tv_sec, ts.tv_nsec / 1000, info);
    printf(buf, var);
    fflush(stdout);
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
