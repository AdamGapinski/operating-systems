#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include "include/Semaphores.h"

int semaphores_id[SEMAPHORE_COUNT];

void initSemaphores(char *pathname) {
    for (int i = 0; i < SEMAPHORE_COUNT; ++i) {
        int semid;
        key_t key;
        if ((key = ftok(pathname, i)) == -1) {
            perror("Error initSemaphores ftok");
            exit(EXIT_FAILURE);
        }
        if ((semid = semget(key, 1, 0600 | IPC_CREAT | IPC_EXCL)) != -1) {
            semaphores_id[i] = semid;
            set_semaphore(i, 0);
        } else if (errno != EEXIST || (semid = semget(key, 1, 0600)) == -1) {
            perror("Error initSemaphores semget");
            exit(EXIT_FAILURE);
        }
        semaphores_id[i] = semid;
    }
}

void removeSemaphores(char *pathname) {
    int semid;
    key_t key;
    for (int i = 0; i < SEMAPHORE_COUNT; ++i) {
        if ((key = ftok(pathname, i)) == -1) {
            if (errno != ENOENT) {
                perror("Error removeSemaphores ftok");
                exit(EXIT_FAILURE);
            }
        } else {
            if ((semid = semget(key, 0, 0)) == -1) {
                if (errno != ENOENT) {
                    perror("Error removeSemaphores semget");
                    exit(EXIT_FAILURE);
                }
            } else {
                if (semctl(semid, 0, IPC_RMID) == -1) {
                    perror("Error semctl IPC_RMID");
                    exit(EXIT_FAILURE);
                };
            }
        }
    }
}

void wait_semaphore(int lock_type) {
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    if (semop(semaphores_id[lock_type], &buf, 1) == -1) {
        perror("Error wait_semaphore");
        exit(EXIT_FAILURE);
    };
}

int nowait_semaphore(int lock_type) {
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = -1;
    buf.sem_flg = IPC_NOWAIT;
    int result = semop(semaphores_id[lock_type], &buf, 1);
    if (result == -1 && errno != EAGAIN) {
        perror("Error nowait_semaphore");
        exit(EXIT_FAILURE);
    }
    return result == -1 ? 0 : 1;
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
        perror("Error wait_semaphore_acquired");
        exit(EXIT_FAILURE);
    };
}
