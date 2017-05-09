#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
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

void wait_lock(int lock_type);

void initSemaphores();

void set_semaphore(int lock_type, int val) ;

int get_semaphore(int lock_type) ;

int get_lock_info(int lock_type, int info_type) ;

ClientsQueue *clientsQueue;

int semaphores_id[SEMAPHORE_COUNT];

int main(int argc, char *argv[]) {
    signal(SIGINT, sigintExitHandler);
    atexit(clean);
    int size = parseClientsQueueSize(argc, argv);
    initSemaphores();
    initClientsQueue(size);

    log_info("Barber started as process with id %d", getpid());

    //todo move to init function
    //set lock of CLIENT and BARBER turn to 0
    while (1) {
        //Barber is checking if anyone is waiting in the queue
        release_lock(BARBER_READY);
        if (clients_queue_is_empty() == 1) {
            log_info("Golibroda zasypia", 0);
            release_lock(BARBER_FREE_TO_WAKE_UP); //+1
            wait_lock(BARBER_READY);
            //clients are free to wake barber up
            //waiting for waking up
            log_info("Golibroda spi bufor %d", get_semaphore(BARBER_FREE_TO_WAKE_UP));
            wait_lock_acquired(BARBER_FREE_TO_WAKE_UP);
            log_info("Golibroda spi bufor %d", get_semaphore(BARBER_FREE_TO_WAKE_UP));
            log_info("DEBUG obudzony", 0);
            //woken up, some client acquired sleeping barber lock (value 0)
            //getting client pid
            int clientID = get_lock_info(BARBER_FREE_TO_WAKE_UP, LAST_SEMOP_PID);
            //barber has to take the done lock to make his job
            wait_lock(BARBER_TURN);
            log_info("DEBUG waited for turn", 0);
            clientID = pid;
            log_info("Strzyzenie klienta o identyfikatorze %d", clientID);
            log_info("Zakonczenie strzyzenia klienta o identyfikatorze %d", clientID);
            //client has been shaved, so Barber releases Chair and client can leave then
            release_lock(CLIENT_TURN);
            wait_lock(BARBER_TURN);
        } else {
            wait_lock(BARBER_READY);
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

void initSemaphores() {
    for (int i = 0; i < SEMAPHORE_COUNT; ++i) {
        semctl(semget(ftok(getenv("HOME"), i), 0, 0), 0, IPC_RMID);
        errno = 0;
        int semid;
        if ((semid = semget(ftok(getenv("HOME"), i), 1, 0666 | IPC_CREAT | IPC_EXCL)) == -1) {
            perror("Error");
            exit(EXIT_FAILURE);
        }
        semaphores_id[i] = semid;

    }

    set_semaphore(BARBER_FREE_TO_WAKE_UP, 0);
    set_semaphore(READING_QUEUE, 1);
    set_semaphore(WRITING_QUEUE, 1);
    set_semaphore(BARBER_TURN, 0);
    set_semaphore(CLIENT_TURN, 0);
    set_semaphore(BARBER_READY, 1);
}

void set_semaphore(int lock_type, int val) {
    union semun {
        int              val;
        struct semid_ds *buf;
        unsigned short  *array;
        struct seminfo  *__buf;
    } semunion;
    semunion.val = val;
    semctl(semaphores_id[lock_type], 0, SETVAL, semunion);
}

int get_semaphore(int lock_type) {
    return semctl(semaphores_id[lock_type], 0, GETVAL);
}

int get_lock_info(int lock_type, int info_type) {
    int pid = -1;
    if (info_type == LAST_SEMOP_PID) {
        pid = semctl(semaphores_id[lock_type], 0, GETPID);
        if (pid == -1) {
            perror("Error");
            exit(EXIT_FAILURE);
        }
    }
    return pid;
}

void wait_lock_acquired(int lock_type) {
    struct sembuf buf;
    buf.sem_op = 0;
    buf.sem_num = 0;
    buf.sem_flg = 0;
    if (semop(semaphores_id[lock_type], &buf, 1) == -1) {
        printf("%d\n", errno);
        perror("Error");
    };
}

void release_lock(int lock_type) {
    set_semaphore(lock_type, 1);
}

void log_info(char *info, int id) {
    char buf[1000];
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    sprintf(buf, "%ld, %s\n", ts.tv_nsec, info);
    printf(buf, id);
    fflush(stdout);
}

int clients_queue_is_empty() {
    int queued;
    wait_lock(READING_QUEUE);
    queued = clientsQueue->queued;
    log_info("clients queue queued clients %d", queued);
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
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    semop(semaphores_id[lock_type], &buf, 1);
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
    log_info("get clients queue queue %d", get_semaphore(READING_QUEUE));
    wait_lock(WRITING_QUEUE);
    log_info("get clients queue queue", 0);
    log_info("get clients queue queue %d", *queued);

    if (*queued == 0) {
        log_info("dup", 0);
        release_lock(WRITING_QUEUE);
        return -1;
    }
    log_info("adup", 0);
    log_info("get clients queue head %d", *head);
    int result = clientsQueue->queue[*head];
    clientsQueue->head = (*head + 1) % size;
    clientsQueue->queued -= 1;
    release_lock(WRITING_QUEUE);
    log_info("get clients queue queue %d", get_semaphore(READING_QUEUE));
    return result;
}

void initClientsQueue(int size) {
    /*
     * getAdr will return a pointer to shared memory, which will be mapped to ClientsQueue structure,
     * but besides that, the memory will also contain array of ints, allocated after this structure.
     * */
    int sizeOfArray = size * sizeof(*(*clientsQueue).queue);
    void *shared = getAdr(sizeof(*clientsQueue) + sizeOfArray);
    clientsQueue = (ClientsQueue *) shared;
    clientsQueue->queue = shared + sizeof(ClientsQueue);
    clientsQueue->head = 0;
    clientsQueue->queued = 0;
    clientsQueue->size = size;
}

void *getAdr(int size) {
    int key;
    if ((key = ftok(getenv("HOME"), CLIENTS_QUEUE_KEY)) == -1) {
        perror("getting key");
        exit(EXIT_FAILURE);
    };
    log_info("DEBUG key %d", key);
    int shId;
    shmctl(shmget(key, 0, 0), IPC_RMID, NULL);
    errno = 0;
    if ((shId = shmget(key, (size_t) size, IPC_CREAT | IPC_EXCL | 0666)) == -1) {
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
    for (int i = 0; i < SEMAPHORE_COUNT; ++i) {
        semctl(semaphores_id[i], 0, IPC_RMID);
    }
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
