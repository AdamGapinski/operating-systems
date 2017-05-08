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

int clients_queue_is_empty();

void log_info(char *info, int id);

void release_lock(int lock_type);

void wait_lock_acquired(int lock_type);

int get_lock_info(int info_type);

void wait_lock(int lock_type);

ClientsQueue *clientsQueue;

int main(int argc, char *argv[]) {
    signal(SIGINT, sigintExitHandler);
    atexit(clean);
    int size = parseClientsQueueSize(argc, argv);
    initSemaphores();
    initClientsQueue(size);

    //todo move to init function
    release_lock(CHAIR_LOCK); //chair is free, setting to 1
    wait_lock(DONE_LOCK); //not done, setting to zero
    while (1) {
        //Barber is checking if anyone is waiting in the queue
        if (clients_queue_is_empty() == 1) {
            /*
             * No one is waiting in the queue, so Barber is going to sleep.
             * Posting sleeping semaphore allows new client to wake him up.
             * */
            release_lock(SLEEPING_BARBER_LOCK);
            log_info("Golibroda zasypia", 0);
            //waiting for waking up
            wait_lock_acquired(SLEEPING_BARBER_LOCK);
            //waiting for client to take a seat
            wait_lock_acquired(CHAIR_LOCK);
            //getting client pid
            int clientID = get_lock_info(LAST_SEMOP_PID);
            log_info("Strzyzenie klienta o identyfikatorze %d", clientID);
            log_info("Zakonczenie strzyzenia klienta o identyfikatorze %d", clientID);
            //client has been shaved, so Barber releases Chair and client can leave then
            release_lock(DONE_LOCK);
        } else {
            //Getting client from queue
            int clientId = dequeue(clientsQueue->queue, &clientsQueue->head, &clientsQueue->queued, clientsQueue->size);
            log_info("Strzyzenie klienta o identyfikatorze %d", clientId);
            log_info("Zakonczenie strzyzenia klienta o identyfikatorze %d", clientId);

            //todo implement this by releasing lock
            kill(clientId, SIGCONT);
            //release_lock(clientId);
        }
        release_lock(CHAIR_LOCK); //chair is free, setting to 1
    }
}

int get_lock_info(int info_type) {
    return 0;
}

void wait_lock_acquired(int lock_type) {

}

void release_lock(int lock_type) {

}

void log_info(char *info, int id) {

}

int clients_queue_is_empty() {
    int queued;
    wait_lock(READING_QUEUE);
    queued = clientsQueue->queued;
    release_lock(READING_QUEUE);
    return queued == 0 ? 1 : 0;
}

int clients_queue_is_full() {
    int queued, size;
    wait_lock(READING_QUEUE);
    queued = clientsQueue->queued;
    size = clientsQueue->size;
    release_lock(READING_QUEUE);
    return queued == size ? 1 : 0;
}

void wait_lock(int lock_type) {

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

int dequeue(int *queue, int *head, int *queued, int size) {
    wait_lock(WRITING_QUEUE);
    if (*queued == 0) {
        release_lock(WRITING_QUEUE);
        return -1;
    }
    int result = queue[*head];
    *head = (*head + 1) % size;
    *queued -= 1;
    release_lock(WRITING_QUEUE);
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
