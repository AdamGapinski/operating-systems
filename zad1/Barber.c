#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include "include/Common.h"

int parseClientsQueueSize(int argc, char *argv[]) ;

void clean();

void sigintExitHandler(int sig) ;

void *getAdr(int i);

int enqueue(int *queue, int *head, int *queued, int size, int value) ;

int dequeue(int *queue, int *head, int *queued, int size) ;

void initClientsQueue(int size) ;

ClientsQueue *clientsQueue;

int main(int argc, char *argv[]) {
    signal(SIGINT, sigintExitHandler);
    atexit(clean);
    int size = parseClientsQueueSize(argc, argv);
    initClientsQueue(size);
}

int enqueue(int *queue, int *head, int *queued, int size, int value) {
    if (*queued == size) {
        return -1;
    }
    queue[(*head + *queued) % size] = value;
    *queued += 1;
    return 0;
}

int dequeue(int *queue, int *head, int *queued, int size) {
    if (*queued == 0) {
        return -1;
    }
    int result = queue[*head];
    *head = (*head + 1) % size;
    *queued -= 1;
    return result;
}

void initClientsQueue(int size) {
    /*
     * getAdr will return a pointer to shared memory, which will be mapped to ClientsQueue structure,
     * but besides that, the memory will also contain array of ints, allocated after this structure.
     * */
    int sizeOfArray = size * sizeof(*(*clientsQueue).queue);
    clientsQueue = (ClientsQueue *) getAdr(sizeof(*clientsQueue) + sizeOfArray);
    clientsQueue->queue = (void *) clientsQueue + sizeof(*clientsQueue);
    clientsQueue->head = 0;
    clientsQueue->queued = 0;
    clientsQueue->size = size;
}

void *getAdr(int size) {
    int key;
    if ((key = ftok("./", CLIENTS_QUEUE_KEY)) == -1) {
        perror("getting key");
        exit(EXIT_FAILURE);
    };
    int shId;

    errno = 0;
    if ((shId = shmget(key, (size_t) size, IPC_CREAT | 0666)) == -1) {
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

void sigintExitHandler(int sig) {
    exit(EXIT_SUCCESS);
}

void clean() {
    shmdt(clientsQueue);
    shmctl(shmget(ftok("./", CLIENTS_QUEUE_KEY), 0, 0), IPC_RMID, NULL);
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
