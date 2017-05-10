#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../include/SharedQueue.h"
#include "../include/Semaphores.h"

ClientsQueue *queue;
char *pathname = "/home/adam";
int proj_id = 205;

void simpleTest() {
    queue = initQueue(pathname, proj_id, 10);
    int process_num = 100;

    for (int i = 0; i < process_num; ++i) {
        if (fork() == 0) {
            while(enqueue(queue, getpid()) == -1);
            printf("enqueued %d\n", getpid());
            fflush(stdout);
            exit(EXIT_SUCCESS);
        }
    }
    int pid;
    sleep(1);
    while ((pid = dequeue(queue)) != -1) {
        sleep(1);
        printf("head %d\n", *queue->head);
        printf("queued %d\n", *queue->queued);
        printf("size %d\n", *queue->size);
        printf("got pid %d\n", pid);
    }
}

void rm_semaphores_at_exit() {
    removeQueue(queue, pathname, proj_id);
    removeSemaphores(pathname);
}

int main(int argc, char *argv[]) {
    removeSemaphores(pathname);
    initSemaphores(pathname);
    release_semaphore(QUEUE_SYNCHRONIZATION);
    simpleTest();
    atexit(rm_semaphores_at_exit);
}
