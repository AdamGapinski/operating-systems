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
#include "include/Common.h"

int parseUIntArgument(int argc, char **argv, int arg_num, char *des);

void start_client(int i);

void start_clients(int clients_count, int shaving_count);

int main(int argc, char *argv[]) {
    int clients_count = parseUIntArgument(argc, argv, 1, "number of clients");
    int shaving_count = parseUIntArgument(argc, argv, 2, "number of shavings");
    start_clients(clients_count, shaving_count);

    //waiting for child processes
    while (waitpid(-1, NULL, 0) != -1);
    exit(EXIT_SUCCESS);
}

void start_clients(int clients_count, int shaving_count) {
    for (int i = 0; i < clients_count; ++i) {
        int pid;
        if ((pid = fork()) == 0) {
            start_client(shaving_count);
        } else if (pid == -1) {
            perror("Error");
            exit(EXIT_FAILURE);
        };
    }
}

void start_client(int shaving_count) {
    while (shaving_count > 0) {

        --shaving_count;
    }

    exit(EXIT_SUCCESS);
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

void *getAdr(int size) {
    int key;
    if ((key = ftok("./", CLIENTS_QUEUE_KEY)) == -1) {
        perror("getting key");
        exit(EXIT_FAILURE);
    };
    int shId;
    errno = 0;
    if ((shId = shmget(key, (size_t) size, 0666)) == -1) {
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
    if (*queued == size) {
        return -1;
    }
    queue[(*head + *queued) % size] = value;
    *queued += 1;
    return 0;
}

int dequeue(int *queue, int *head, int *queued, int size) {
    if (*queued == 0) {
        return -1;
    }
    int result = queue[*head];
    *head = (*head + 1) % size;
    *queued -= 1;
    return result;
}
