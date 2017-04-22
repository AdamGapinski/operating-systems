#include <stdio.h>
#include "Common.h"

int main() {
    printf("%d: Server started\n", getpid());

    int queue_id = create_queue();

    printf("Hello, World!\n");
    return 0;
}

int create_queue() {
    printf("%d: Server is creating message queue\n", getpid());
    int queue_id;
    if ((queue_id = msgget(IPC_PRIVATE, 0666)) == -1) {
        perror("Error while creating server queue");
        exit(EXIT_FAILURE);
    }
    printf("%d: Server created message queue with id %d\n", getpid(), queue_id);
    return queue_id;
}