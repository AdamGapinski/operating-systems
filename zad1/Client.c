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

int parseUIntArgument(int argc, char **argv, int arg_num, char *des);

void start_client(int i);

void start_clients(int clients_count, int shaving_count);

int queue_up();

void *getClientsQueue() ;

int try_lock(int lock_type);

void log_info(char *info, int id) ;

void wait_lock(int lock_type);

void release_lock(int lock_type) ;

int wait_to_unlock(int lock_type);

void initSemaphores();

void set_semaphore(int lock_type, int val);

int get_semaphore(int lock_type) ;

int enqueue(int *queue, int *head, int *queued, int size, int value) ;

void wait_lock_acquired(int lock_type) ;

ClientsQueue *clientsQueue;
int *queue;

int semaphores_id[SEMAPHORE_COUNT];

int main(int argc, char *argv[]) {
    int clients_count = parseUIntArgument(argc, argv, 1, "number of clients");
    int shaving_count = parseUIntArgument(argc, argv, 2, "number of shavings");
    initSemaphores();

    void *shared = getClientsQueue();
    clientsQueue = (ClientsQueue *) shared;
    clientsQueue->queue = shared + sizeof(ClientsQueue);
    start_clients(clients_count, shaving_count);
    //waiting for child processes
    log_info("DEBUG waitpid", 0);
    int status;
    while (waitpid(-1, &status, 0) != -1);
    if (WIFSIGNALED(status)) {
        log_info("wifsignaled %d", WTERMSIG(status));
    } else if (WIFEXITED(status)) {
        log_info("wifexited %d", WEXITSTATUS(status));
    }
    log_info("DEBUG waitedpid", 0);
    exit(EXIT_SUCCESS);
}

void initSemaphores() {
    for (int i = 0; i < SEMAPHORE_COUNT; ++i) {
        int semid;
        if ((semid = semget(ftok(getenv("HOME"), i), 1, 0)) == -1) {
            perror("Error while getting Barber semaphores");
            exit(EXIT_FAILURE);
        }
        semaphores_id[i] = semid;
    }
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

void start_clients(int clients_count, int shaving_count) {
    for (int i = 0; i < clients_count; ++i) {
        int pid;
        if ((pid = fork()) == 0) {
            log_info("DEBUG forked %d", getpid());
            start_client(shaving_count);
        } else if (pid == -1) {
            perror("Error");
            exit(EXIT_FAILURE);
        };
    }
}

void start_client(int shaving_count) {
    while (shaving_count > 0) {
        log_info("DEBUG shaving count %d", shaving_count);
        wait_lock_acquired(BARBER_READY);
        if (try_lock(BARBER_FREE_TO_WAKE_UP) == 1) {
            log_info("DEBUG obudzil", 0);
            /*If client can acquire sleeping barber semaphore, then it means that the barber was sleeping and
             * the client has woken him up and will be shaved*/
            log_info("Golibroda obudzony", 0);
            pid = getpid();
            //client is being shaved
            release_lock(BARBER_TURN);
            log_info("DEBUG released barber", 0);
            wait_lock(CLIENT_TURN);
            log_info("Klient %d opuszcza zaklad po zakonczeniu strzyzenia", getpid());
            release_lock(BARBER_TURN);
            --shaving_count;
        } else {
            /*If client could not acquire sleeping barber semaphore, then it means that the barber is busy and
             * the client has to wait in the queue*/
            log_info("DEBUG nie obudzil", 0);
            if (queue_up() == 1) {
                log_info("DEBUG stanal w kolejce", 0);
                //todo check if strzyzenie juz sie nie zaczelo
                log_info("Klient %d zajal miejsce w poczekalni", getpid());
                //Client has found place in queue, and he is waiting to get shaved
                //todo implement this by releasing lock
                //wait_lock(getpid());
                //raise(SIGSTOP);
                log_info("Klient %d opuszcza zaklad po zakonczeniu strzyzenia", getpid());
                //Client was shaved and now he is leaving
            } else {
                log_info("DEBUG nie stanal w kolejce", 0);
                log_info("Klient %d opuszcza zaklad z powodu braku wolnych miejsc w poczekalni", getpid());
                //Client could not find place in queue, so he is leaving
            }
        };
    }

    //Client was shaved shaving_count times, so the process is exiting with success.
    exit(EXIT_SUCCESS);
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

void wait_lock(int lock_type) {
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = -1;
    buf.sem_flg = SEM_UNDO;
    semop(semaphores_id[lock_type], &buf, 1);
}

void release_lock(int lock_type) {
    set_semaphore(lock_type, 1);
}

int try_lock(int lock_type) {
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = -1;
    buf.sem_flg = IPC_NOWAIT;
    errno = 0;
    int returnvalue = semop(semaphores_id[lock_type], &buf, 1);
    if (returnvalue == -1) {
        printf("%d\n", errno);
        fflush(stdin);
        perror("Error");
    }
    return returnvalue == -1 ? 0 : 1;
}

int queue_up() {
    enqueue(clientsQueue->queue, &clientsQueue->head, &clientsQueue->queued, clientsQueue->size, getpid());
    return 0;
}

void log_info(char *info, int id) {
    char buf[1000];
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    sprintf(buf, "%ld, %s\n", ts.tv_nsec, info);
    printf(buf, id);
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

void *getClientsQueue() {
    int key;
    if ((key = ftok(getenv("HOME"), CLIENTS_QUEUE_KEY)) == -1) {
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
    log_info("get clients queue queue %d", get_semaphore(WRITING_QUEUE));
    wait_lock(WRITING_QUEUE);
    if (*queued == size) {
        release_lock(WRITING_QUEUE);
        return -1;
    }
    log_info("get clients queue queue %d", get_semaphore(WRITING_QUEUE));
    queue[(*head + *queued) % size] = value;
    log_info("get clients queue queue %d", get_semaphore(WRITING_QUEUE));
    *queued += 1;
    release_lock(WRITING_QUEUE);
    log_info("get clients queue queue %d", get_semaphore(READING_QUEUE));
    return 0;
}

int get_semaphore(int lock_type) {
    return semctl(semaphores_id[lock_type], 0, GETVAL);
}