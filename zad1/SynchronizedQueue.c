#include "include/Semaphores.h"

int clients_queue_is_empty(int *queued) {
    int l_queued;
    wait_semaphore(QUEUE_SYNCHRONIZATION);
    l_queued = *queued;
    release_semaphore(QUEUE_SYNCHRONIZATION);
    return l_queued == 0 ? 1 : 0;
}

int clients_queue_is_full(int *queued, int *size) {
    int l_queued, l_size;
    wait_semaphore(QUEUE_SYNCHRONIZATION);
    l_queued = *queued;
    l_size = *size;
    release_semaphore(QUEUE_SYNCHRONIZATION);
    return queued == size ? 1 : 0;
}

int enqueue(int *queue, int *head, int *queued, int size, int value) {
    wait_semaphore(QUEUE_SYNCHRONIZATION);
    if (*queued == size) {
        release_semaphore(QUEUE_SYNCHRONIZATION);
        return -1;
    }
    queue[(*head + *queued) % size] = value;
    *queued += 1;
    release_semaphore(QUEUE_SYNCHRONIZATION);
    return 0;
}

int dequeue(int *queue, int *head, int *queued, int size) {
    wait_semaphore(QUEUE_SYNCHRONIZATION);
    if (*queued == 0) {
        release_semaphore(QUEUE_SYNCHRONIZATION);
        return -1;
    }
    int result = queue[*head];
    *head = (*head + 1) % size;
    *queued -= 1;
    release_semaphore(QUEUE_SYNCHRONIZATION);
    return result;
}
