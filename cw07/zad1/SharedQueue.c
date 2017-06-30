#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "include/Semaphores.h"
#include "include/SharedQueue.h"

ClientsQueue *initQueue(char *pathname, int proj_id, int size) {
    ClientsQueue *queue = malloc(sizeof(*queue));
    void *adr = get_shared_address(sizeof(*queue) + size * sizeof(*queue->array), pathname, proj_id);
    queue->head = adr;
    queue->queued = adr + sizeof(queue->head);
    queue->size = adr + sizeof(queue->head) + sizeof(queue->queued);
    queue->array = adr + sizeof(queue->head) + sizeof(queue->queued) + sizeof(queue->size);

    if (size != 0) {
        *queue->head = 0;
        *queue->queued = 0;
        *queue->size = size;
    }
    return queue;
}

void removeQueue(ClientsQueue *queue, char *pathname, int proj_id) {
    remove_shared_address(pathname, proj_id, queue->head);
    remove_shared_address(pathname, proj_id, queue->queued);
    remove_shared_address(pathname, proj_id, queue->size);
    remove_shared_address(pathname, proj_id, queue->array);
    free(queue);
}

int clients_queue_is_empty(ClientsQueue *queue) {
    int l_queued;
    wait_semaphore(QUEUE_SYNCHRONIZATION);
    l_queued = *queue->queued;
    release_semaphore(QUEUE_SYNCHRONIZATION);
    return l_queued == 0 ? 1 : 0;
}

int clients_queue_is_full(ClientsQueue *queue) {
    int l_queued, l_size;
    wait_semaphore(QUEUE_SYNCHRONIZATION);
    l_queued = *queue->queued;
    l_size = *queue->size;
    release_semaphore(QUEUE_SYNCHRONIZATION);
    return l_queued == l_size ? 1 : 0;
}

int enqueue(ClientsQueue *queue, int value) {
    wait_semaphore(QUEUE_SYNCHRONIZATION);
    if (*queue->queued == *queue->size) {
        release_semaphore(QUEUE_SYNCHRONIZATION);
        return -1;
    }
    queue->array[(*queue->head + *queue->queued) % *queue->size] = value;
    *queue->queued += 1;
    release_semaphore(QUEUE_SYNCHRONIZATION);
    return 0;
}

int dequeue(ClientsQueue *queue) {
    wait_semaphore(QUEUE_SYNCHRONIZATION);
    if (*queue->queued == 0) {
        release_semaphore(QUEUE_SYNCHRONIZATION);
        return -1;
    }
    int result = queue->array[*queue->head];
    *queue->head = (*queue->head + 1) % *queue->size;
    *queue->queued -= 1;
    release_semaphore(QUEUE_SYNCHRONIZATION);
    return result;
}

void remove_shared_address(char *pathname, int proj_id, void *adr) {
    shmdt(adr);
    errno = 0;
    int sh_mem_id;
    key_t key;
    if ((key = ftok(pathname, proj_id)) == -1) {
        if (errno != ENOENT) {
            perror("Error remove_shared_address ftok");
            exit(EXIT_FAILURE);
        }
    } else {
        if ((sh_mem_id = shmget(key, 0, 0)) == -1) {
            if (errno != ENOENT) {
                perror("Error remove_shared_address shmget");
                exit(EXIT_FAILURE);
            }
        } else {
            if (shmctl(sh_mem_id, 0, IPC_RMID) == -1) {
                perror("Error remove_shared_address shmctl IPC_RMID");
                exit(EXIT_FAILURE);
            };
        }
    }
}

void *get_shared_address(int size, char *pathname, int proj_id) {
    int key;
    if ((key = ftok(pathname, proj_id)) == -1) {
        perror("Error get_shared_address ftok");
        exit(EXIT_FAILURE);
    };

    int sh_mem_id;
    if ((sh_mem_id = shmget(key, (size_t) size, 0600 | IPC_CREAT | IPC_EXCL)) == -1) {
        if (errno != EEXIST || (sh_mem_id = shmget(key, (size_t) size, 0600)) == -1) {
            perror("Error get_shared_address shmget id");
            exit(EXIT_FAILURE);
        }
    }

    void *result;
    if ((result = shmat(sh_mem_id, NULL, 0)) == (void *) -1) {
        perror("Error get_shared_address shamt");
        exit(EXIT_FAILURE);
    }
    return result;
}
