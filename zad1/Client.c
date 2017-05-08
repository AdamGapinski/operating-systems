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
#include "include/Common.h"

int parseUIntArgument(int argc, char **argv, int arg_num, char *des);

void start_client(int i);

void start_clients(int clients_count, int shaving_count);

int queue_up();

void *getClientsQueue() ;

int enqueue(int *queue, int *head, int *queued, int size, int value) ;

int try_lock(int lock_type);

void log_info(char *info, int id) ;

void wait_lock(int lock_type);

void release_lock(int lock_type) ;

ClientsQueue *clientsQueue;

int main(int argc, char *argv[]) {
    int clients_count = parseUIntArgument(argc, argv, 1, "number of clients");
    int shaving_count = parseUIntArgument(argc, argv, 2, "number of shavings");
    clientsQueue = (ClientsQueue *) getClientsQueue();
    start_clients(clients_count, shaving_count);
    //waiting for child processes
    while (waitpid(-1, NULL, 0) != -1);
    exit(EXIT_SUCCESS);
}

void start_clients(int clients_count, int shaving_count) {
    for (int i = 0; i < clients_count; ++i) {
        int pid;
        if ((pid = fork()) == 0) {
            start_client(shaving_count);
        } else if (pid == -1) {
            perror("Error");
            exit(EXIT_FAILURE);
        };
    }
}

void start_client(int shaving_count) {
    while (shaving_count > 0) {
        if (check_unlock(BARBER_READY) && try_lock(SLEEPING_BARBER_LOCK) == 1) {
            /*If client can acquire sleeping barber semaphore, then it means that the barber was sleeping and
             * the client has woken him up and will be shaved*/
            log_info("Golibroda obudzony", 0);
            //client is being shaved
            release_lock(BARBER_TURN);
            wait_lock(CLIENT_TURN);
            log_info("Klient %d opuszcza zaklad po zakonczeniu strzyzenia", getpid());
            release_lock(BARBER_TURN);
            --shaving_count;
        } else {
            /*If client could not acquire sleeping barber semaphore, then it means that the barber is busy and
             * the client has to wait in the queue*/
            if (queue_up() == 1) {
                //todo check if strzyzenie juz sie nie zaczelo
                log_info("Klient %d zajal miejsce w poczekalni", getpid());
                //Client has found place in queue, and he is waiting to get shaved
                //todo implement this by releasing lock
                //wait_lock(getpid());
                raise(SIGSTOP);
                log_info("Klient %d opuszcza zaklad po zakonczeniu strzyzenia", getpid());
                //Client was shaved and now he is leaving
            } else {
                log_info("Klient %d opuszcza zaklad z powodu braku wolnych miejsc w poczekalni", getpid());
                //Client could not find place in queue, so he is leaving
            }
        };
    }

    //Client was shaved shaving_count times, so the process is exiting with success.
    exit(EXIT_SUCCESS);
}

void wait_lock(int lock_type) {

}

void release_lock(int lock_type) {

}

int try_lock(int lock_type) {
    return 0;
}

int queue_up() {
    enqueue(clientsQueue->queue, &clientsQueue->head, &clientsQueue->queued, clientsQueue->size, getpid());
    return 0;
}

void log_info(char *info, int id) {

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

void *getClientsQueue() {
    int key;
    if ((key = ftok("./", CLIENTS_QUEUE_KEY)) == -1) {
        perror("getting key");
        exit(EXIT_FAILURE);
    };
    int shId;
    errno = 0;
    if ((shId = shmget(key, 0, 0666)) == -1) {
        perror("getting shared memory id");
        exit(EXIT_FAILURE);
    }
    void *result;
    if ((result = shmat(shId, NULL, 0)) == (void *) -1) {
        perror("attaching memory");
        exit(EXIT_FAILURE);
    }
    return result;
}

int enqueue(int *queue, int *head, int *queued, int size, int value) {
    wait_lock(WRITING_QUEUE);
    if (*queued == size) {
        release_lock(WRITING_QUEUE);
        return -1;
    }
    queue[(*head + *queued) % size] = value;
    *queued += 1;
    release_lock(WRITING_QUEUE);
    return 0;
}
