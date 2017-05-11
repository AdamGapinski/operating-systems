#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "include/Semaphores.h"
#include "include/SharedQueue.h"

int shared_mem_fd;

ClientsQueue *initQueue(char *global_name, int size) {
    ClientsQueue *queue = malloc(sizeof(*queue));
    void *adr = get_shared_address(sizeof(*queue) + size * sizeof(*queue->array), global_name);
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

void removeQueue(ClientsQueue *queue, char *global_name) {
    remove_shared_address(global_name, queue->array, *queue->size);
    remove_shared_address(global_name, queue->head, sizeof(*queue->head));
    remove_shared_address(global_name, queue->queued, sizeof(*queue->queued));
    remove_shared_address(global_name, queue->size, sizeof(*queue->size));
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

void *get_shared_address(int size, char *global_name) {
    if ((shared_mem_fd = shm_open(global_name, O_RDWR | O_CREAT, 0600)) == -1) {
        perror("Error getting shared memory id");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(shared_mem_fd, size) == -1) {
        perror("Error ftruncate");
        exit(EXIT_FAILURE);
    }
    void *result;
    if ((result = mmap(NULL, (size_t) size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem_fd, 0)) == MAP_FAILED) {
        perror("Error mapping shared memory");
        exit(EXIT_FAILURE);
    }
    return result;
}

void remove_shared_address(char *global_name, void *adr, int size) {
    munmap(adr, (size_t) size);
    close(shared_mem_fd);
    errno = 0;
    if (shm_unlink(global_name) == -1) {
        if (errno != ENOENT) {
            perror("Error memory unlink");
            exit(EXIT_FAILURE);
        }
    }
}
