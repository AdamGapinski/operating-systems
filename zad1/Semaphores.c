#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include "include/Semaphores.h"
#include "include/Common.h"

int semaphores_id[SEMAPHORE_COUNT];

void wait_semaphore(int lock_type) {
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    if (semop(semaphores_id[lock_type], &buf, 1) == -1) {
        perror("Error wait_lock");
        exit(EXIT_FAILURE);
    };
}

void release_semaphore(int lock_type) {
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = 1;
    buf.sem_flg = 0;
    if (semop(semaphores_id[lock_type], &buf, 1) == -1) {
        perror("Error release_semaphore");
        exit(EXIT_FAILURE);
    };
}

void set_semaphore(int lock_type, int val) {
    union semun {
        int              val;
        struct semid_ds *buf;
        unsigned short  *array;
        struct seminfo  *__buf;
    } semunion;
    semunion.val = val;
    if (semctl(semaphores_id[lock_type], 0, SETVAL, semunion) == -1) {
        perror("Error set_semaphore");
        exit(EXIT_FAILURE);
    };
}

int get_semaphore(int lock_type) {
    int result;
    if ((result = semctl(semaphores_id[lock_type], 0, GETVAL)) == -1) {
        perror("Error get_semaphore");
        exit(EXIT_FAILURE);
    }
    return result;
}

int get_lock_info(int lock_type, int info_type) {
    int pid = -1;
    if (info_type == LAST_SEMOP_PID) {
        pid = semctl(semaphores_id[lock_type], 0, GETPID);
        if (pid == -1) {
            perror("Error get_lock_info GETPID");
            exit(EXIT_FAILURE);
        }
    }
    return pid;
}

void wait_semaphore_acquired(int lock_type) {
    struct sembuf buf;
    buf.sem_op = 0;
    buf.sem_num = 0;
    buf.sem_flg = 0;
    if (semop(semaphores_id[lock_type], &buf, 1) == -1) {
        perror("Error wait_lock_acquired");
        exit(EXIT_FAILURE);
    };
}
