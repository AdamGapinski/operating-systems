#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <memory.h>
#include <time.h>
#include "include/Semaphores.h"

sem_t *semaphores_adr[SEMAPHORE_COUNT];

char *get_sem_name(char *global_name, int i) ;

void initSemaphores(char *global_name) {
    for (int i = 0; i < SEMAPHORE_COUNT; ++i) {
        sem_t *sem_adr;
        char *sem_name = get_sem_name(global_name, i);
        if ((sem_adr = sem_open(sem_name, O_CREAT, 0600, 0)) == SEM_FAILED) {
            perror("Error initSemaphores sem_open");
            exit(EXIT_FAILURE);
        }
        semaphores_adr[i] = sem_adr;
        free(sem_name);
    }
}

void removeSemaphores(char *global_name) {
    for (int i = 0; i < SEMAPHORE_COUNT; ++i) {
        sem_close(semaphores_adr[i]);
        errno = 0;
        char *sem_name = get_sem_name(global_name, i);
        if (sem_unlink(sem_name) == -1 && errno != ENOENT) {
            perror("Error sem_unlink");
            exit(EXIT_FAILURE);
        };
        free(sem_name);
    }
}

void wait_semaphore(int lock_type) {
    if (sem_wait(semaphores_adr[lock_type]) == -1) {
        perror("Error sem_wait");
        exit(EXIT_FAILURE);
    }
}

void timed_wait_semaphore(int lock_type, int sec) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += sec;
    if (sem_timedwait(semaphores_adr[lock_type], &ts) == -1) {
        if (errno != ETIMEDOUT) {
            perror("Error sem_timedwait");
            exit(EXIT_FAILURE);
        } else {
            errno = 0;
        }
    }
}

int nowait_semaphore(int lock_type) {
    int result = sem_trywait(semaphores_adr[lock_type]);
    if (result == -1 && errno != EAGAIN) {
        perror("Error nowait_semaphore");
        exit(EXIT_FAILURE);
    }
    return result == -1 ? 0 : 1;
}

void release_semaphore(int lock_type) {
    if (sem_post(semaphores_adr[lock_type]) == -1) {
        perror("Error release_semaphore");
        exit(EXIT_FAILURE);
    };
}

char *get_sem_name(char *global_name, int i) {
    char *sem_name = calloc(strlen(global_name) + 4, sizeof(*sem_name));
    sprintf(sem_name, "%s%d", global_name, i);
    return sem_name;
}