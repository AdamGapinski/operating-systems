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

int main(int argc, char *argv[]) {
    int size = parseClientsQueueSize(argc, argv);
    init(size);
    log_info("Barber started as process with id %d", getpid());

    while (1) {
        //Barber is checking if anyone is waiting in the queue
        if (clients_queue_is_empty() == 1) {
            log_info("Golibroda zasypia", 0);
            release_semaphore(BARBER_FREE_TO_WAKE_UP); //+1
            wait_semaphore(BARBER_READY);
            //clients are free to wake barber up
            //waiting for waking up
            log_info("Golibroda spi bufor %d", get_semaphore(BARBER_FREE_TO_WAKE_UP));
            wait_semaphore_acquired(BARBER_FREE_TO_WAKE_UP);
            log_info("Golibroda spi bufor %d", get_semaphore(BARBER_FREE_TO_WAKE_UP));
            log_info("DEBUG obudzony", 0);
            //woken up, some client acquired sleeping barber lock (value 0)
            //getting client pid
            int clientID = get_lock_info(BARBER_FREE_TO_WAKE_UP, LAST_SEMOP_PID);
            //barber has to take the done lock to make his job
            wait_semaphore(BARBER_TURN);
            log_info("DEBUG waited for turn", 0);
            clientID = pid;
            log_info("Strzyzenie klienta o identyfikatorze %d", clientID);
            log_info("Zakonczenie strzyzenia klienta o identyfikatorze %d", clientID);
            //client has been shaved, so Barber releases Chair and client can leave then
            release_semaphore(CLIENT_TURN);
            wait_semaphore(BARBER_TURN);
        } else {
            wait_semaphore(BARBER_READY);
            log_info("From queue", 0);
            //Getting client from queue
            int clientId = dequeue(clientsQueue->queue, &clientsQueue->head, &clientsQueue->queued, clientsQueue->size);
            //todo wait until the client said that is waiting in the queue
            log_info("Strzyzenie klienta o identyfikatorze %d", clientId);
            log_info("Zakonczenie strzyzenia klienta o identyfikatorze %d", clientId);

            //todo implement this by releasing lock
            //kill(clientId, SIGCONT);
            //release_lock(clientId);
        }
    }
}

void init(int queue_size) {
    signal(SIGINT, sigintExitHandler);
    atexit(clean);

    removeSemaphores(PATHNAME);
    initSemaphores(PATHNAME);
    clientsQueue = initQueue(PATHNAME, CLIENTS_QUEUE_KEY, queue_size);
    release_semaphore(QUEUE_SYNCHRONIZATION);
}


void sigintExitHandler(int sig) {
    exit(EXIT_SUCCESS);
}

void clean() {
    removeSemaphores(PATHNAME);
    removeQueue(clientsQueue, PATHNAME, CLIENTS_QUEUE_KEY);
}

void log_info(char *info, int var) {
    char buf[strlen(info) + 256];
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    sprintf(buf, "%ld s %ld ns, %s\n\0", ts.tv_sec, ts.tv_nsec, info);
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
