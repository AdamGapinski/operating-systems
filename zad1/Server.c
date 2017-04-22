#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include "common.h"

int create_queue() ;

void handle_request(int queue_id) ;

Request receive_request(int id) ;

void process_request(Request request) ;

void handle_caps();

void handle_time();

void handle_exit();

void handle_echo() ;

void handle_register() ;

int main() {
    printf("%d: Server started\n", getpid());

    int queue_id = create_queue();
    printf("%d: Server is ready to handle requests\n", getpid());

    while(1) {
        handle_request(queue_id);
    }
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

void handle_request(int queue_id) {
    Request request = receive_request(queue_id);
    process_request(request);
    printf("%d: Server ended handling request\n", getpid());
}

Request receive_request(int id) {
    printf("%d: Server is waiting for request\n", getpid());
    sleep(1);
    printf("%d: Server received request\n", getpid());
}

void process_request(Request request) {
    switch (request) {
        case REGISTER:
            printf("%d: Server is registering client\n", getpid());
            handle_register();
            printf("%d: Server registered client\n", getpid());
            break;
        case ECHO:
            printf("%d: Server is processing %s request\n", getpid(), "echo");
            handle_echo();
            printf("%d: Server processed %s request\n", getpid(), "echo");
            break;
        case CAPS:
            printf("%d: Server is processing %s request\n", getpid(), "caps");
            handle_caps();
            printf("%d: Server processed %s request\n", getpid(), "caps");
            break;
        case TIME:
            printf("%d: Server is processing %s request\n", getpid(), "time");
            handle_time();
            printf("%d: Server processed %s request\n", getpid(), "time");
            break;
        case EXIT:
            printf("%d: Server is processing %s request\n", getpid(), "exit");
            handle_exit();
            printf("%d: Server processed %s request\n", getpid(), "exit");
            break;
        default:
            printf("%d: Unrecognized request\n", getpid());
    }
}

void handle_register() {

}

void handle_echo() {

}

void handle_caps() {

}

void handle_time() {

}

void handle_exit() {

}
