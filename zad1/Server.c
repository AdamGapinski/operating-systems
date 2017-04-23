#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

#define MAX_CLIENTS 20

int create_queue() ;

void handle_request() ;

void handle_caps(message *pMessage);

void handle_time(message *pMessage);

void handle_exit();

void handle_echo(message *pMessage);

message *receive_message() ;

void process_message(message *msg) ;

void send_to_client(pid_t pid, long id, char *message) ;

void handle_register(message *msg) ;

typedef struct client {
    pid_t process_id;
    long id;
    int queue_id;
} client;

client *clients[MAX_CLIENTS];
int client_index = 0;

//todo  make non static
int queue_id = 0;

int main() {
    printf("%d: Server started\n", getpid());

    queue_id = create_queue();
    printf("%d: Server is ready to handle requests\n", getpid());

    while(1) {
        handle_request();
    }
}

int create_queue() {
    printf("%d: Server is creating message queue\n", getpid());

    int queue_id;
    if ((queue_id = msgget(ftok(getenv("HOME"), PROJ_ID), IPC_CREAT | 0666)) == -1) {
        perror("Error while creating server queue");
        exit(EXIT_FAILURE);
    }
    printf("%d: Server created message queue with id %d\n", getpid(), queue_id);
    return queue_id;
}

void handle_request() {
    printf("%d: Server is waiting for request\n", getpid());
    message *msg = receive_message();
    process_message(msg);
    printf("%d: Server ended handling request\n", getpid());
}

message *receive_message() {
    message *msg = calloc(1, sizeof(*msg));

    /*
     * todo
     * If everything will be working fine, this could be implemented asynchronously:
     * We wait for different message types in separate processes and process it immediately
     * */
    printf("%d: Server is waiting for message\n", getpid());
    if ((msgrcv(queue_id, msg, sizeof(msg) - sizeof(long), 0, MSG_NOERROR)) == -1) {
        perror("Error while receiving message");
    };
    printf("%d: Server received message\n", getpid());
    return msg;
}

void process_message(message *msg) {
    switch (msg->message_type) {
        case REGISTER:
            printf("%d: Server is registering client\n", getpid());
            handle_register(msg);
            printf("%d: Server registered client\n", getpid());
            break;
        case ECHO:
            printf("%d: Server is processing %s request\n", getpid(), "echo");
            handle_echo(msg);
            printf("%d: Server processed %s request\n", getpid(), "echo");
            break;
        case CAPS:
            printf("%d: Server is processing %s request\n", getpid(), "caps");
            handle_caps(msg);
            printf("%d: Server processed %s request\n", getpid(), "caps");
            break;
        case TIME:
            printf("%d: Server is processing %s request\n", getpid(), "time");
            handle_time(msg);
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
    free(msg);
}

void handle_register(message *msg) {
    //todo deallocate client
    client *to_register = malloc(sizeof(*to_register));
    to_register->process_id = msg->client;
    to_register->id = client_index;
    sscanf(msg->message, "%d", &to_register->queue_id);
    clients[client_index] = to_register;
    printf("%d: Client registered at %d id\n", getpid(), client_index);

    ++client_index;
    printf("%d: Server is sending back ID %ld to client PID: %d\n", getpid(), to_register->id, to_register->process_id);
    send_to_client(to_register->process_id, to_register->id, "");
}

void send_to_client(pid_t pid, long id, char *text) {
    //todo deallocate response
    message *response = malloc(sizeof(*response));
    response->client = pid;
    response->message_type = id;
    strncpy(response->message, text, MSG_MAX_SIZE);

    printf("%d: Server is sending message \"%s\" to %d\n", getpid(), text, pid);
    if ((msgsnd(clients[id]->queue_id, response, sizeof(*response) - sizeof(long), 0)) == -1) {
        perror("Error while responding to register");
    }
    printf("%d: Message sent\n", getpid());
}

void handle_echo(message *pMessage) {

}

void handle_caps(message *pMessage) {

}

void handle_time(message *pMessage) {

}

void handle_exit() {

}
