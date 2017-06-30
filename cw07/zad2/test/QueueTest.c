#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../include/SharedQueue.h"
#include "../include/Semaphores.h"

ClientsQueue *initQueue(int size) {
    ClientsQueue *queue = malloc(sizeof(*queue));
    queue->size = malloc(sizeof(*queue->size));
    queue->head = malloc(sizeof(*queue->head));
    queue->queued = malloc(sizeof(*queue->queued));
    queue->array = calloc((size_t) size, sizeof(*queue->size));
    *queue->size = size;
    *queue->head = 0;
    *queue->queued = 0;
    return queue;
}

void enqueu_members(ClientsQueue *queue, int members) {
    for (int i = 0; i < members; ++i) {
        if (enqueue(queue, i) == -1) {
            printf("Test failed");
            exit(EXIT_SUCCESS);
        }
    }
}

void dequeu_members(ClientsQueue *queue, int members) {
    for (int i = 0; i < members; ++i) {
        int result;
        if ((result = dequeue(queue)) == -1) {
            printf("Test failed");
            exit(EXIT_SUCCESS);
        } else {
            printf("%d ", result);
        }
    }
    printf("\n");
}

void enqueu_and_dequeue(ClientsQueue *queue, int members) {
    enqueu_members(queue, members);
    dequeu_members(queue, members);
}

void simpleTest() {
    ClientsQueue *queue = initQueue(10);

    enqueu_and_dequeue(queue, 10);
    enqueu_and_dequeue(queue, 3);
    enqueu_and_dequeue(queue, 2);
    enqueu_and_dequeue(queue, 8);
    enqueu_and_dequeue(queue, 1);
    enqueu_and_dequeue(queue, 2);
    enqueu_and_dequeue(queue, 10);
    enqueu_and_dequeue(queue, 10);
    enqueu_and_dequeue(queue, 9);

    for (int i = 0; i < 10; ++i) {
        if (dequeue(queue) != -1) {
            printf("Test failed");
            exit(EXIT_SUCCESS);
        }
    }

    enqueu_members(queue, 3);
    dequeu_members(queue, 1);

    enqueu_members(queue, 2);
    dequeu_members(queue, 4);

    enqueu_members(queue, 9);
    dequeu_members(queue, 4);
    enqueu_members(queue, 5);
    dequeu_members(queue, 10);

    free(queue->array);
    free(queue->head);
    free(queue->size);
    free(queue->queued);
    free(queue);
}

void rm_semaphores_at_exit() {
    removeSemaphores("./");
}

int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);
    printf("main pid %d\n", getpid());
    removeSemaphores("./");
    initSemaphores("./");
    release_semaphore(QUEUE_SYNCHRONIZATION);
    simpleTest();
    atexit(rm_semaphores_at_exit);
}


