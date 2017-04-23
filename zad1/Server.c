#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include "common.h"

int create_queue() ;

void handle_request() ;

void handle_caps(message *pMessage);

void handle_time(message *pMessage);

void handle_echo(message *pMessage);

message *wait_for_message() ;

void process_message(message *msg) ;

void send_message(message *to_send) ;

void handle_register(message *msg) ;

message *create_message(pid_t client_pid, char *text) ;

void handle_exit(message *pMessage) ;

void handle_pending_requests();

message *receive_message() ;

typedef struct client {
    pid_t process_id;
    long id;
    int queue_id;
} client;

client *clients[MAX_CLIENTS + 1];
//skipping 0 index to avoid sending message with 0 message type
int client_index = 1;

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
    message *msg = wait_for_message();
    process_message(msg);
    free(msg);
    printf("%d: Server ended handling request\n", getpid());
}

message *wait_for_message() {
    message *msg = calloc(1, sizeof(*msg));

    printf("%d: Server is waiting for message\n", getpid());
    if ((msgrcv(queue_id, msg, sizeof(*msg) - sizeof(long), 0, MSG_NOERROR)) == -1) {
        perror("Error while receiving message");
    } else {
        printf("%d: Server received message \"%s\" with type %ld and PID %d\n", getpid(), msg->message, msg->message_type, msg->client);
    }
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
            handle_exit(msg);
            printf("%d: Server processed %s request\n", getpid(), "exit");
            break;
        default:
            printf("%d: Unrecognized request\n", getpid());
    }
}

void handle_register(message *msg) {
    if (client_index > MAX_CLIENTS) {
        fprintf(stderr, "Error: Server could not register client with PID %d, because no free slots are available\n", msg->client);
        return;
    }
    client *to_register = malloc(sizeof(*to_register));
    to_register->process_id = msg->client;
    to_register->id = client_index;

    /*opening client queue and getting it's identifier*/
    int key;
    sscanf(msg->message, "%d", &key);
    printf("%d: opening client queue with key %d\n", getpid(), key);
    if ((to_register->queue_id = msgget(key, 0666)) == -1) {
        perror("Error while opening client queue");
    };

    clients[client_index] = to_register;
    printf("%d: Client registered at %d id\n", getpid(), client_index);
    ++client_index;

    printf("%d: Server is sending back ID %ld to client PID: %d\n", getpid(), to_register->id, to_register->process_id);
    message *response = create_message(to_register->process_id, "");
    send_message(response);
    free(response);
}

message *create_message(pid_t client_pid, char *text) {
    message *result = malloc(sizeof(*result));
    strncpy(result->message, text, MSG_MAX_SIZE);
    result->client = client_pid;
    return result;
}

void send_message(message *to_send) {
    int id;
    int found = 0;
    for (id = 1 ; id < client_index; ++id) {
        if (clients[id]->process_id == to_send->client) {
            found = 1;
            break;
        }
    }

    if (found == 0) {
        fprintf(stderr, "Error while sending message to client: Client with PID %d is not registered\n", to_send->client);
    } else {
        to_send->message_type = id;
        printf("%d: Server is sending message \"%s\" to PID %d with queue ID: %d\n", getpid(), to_send->message, to_send->client, clients[id]->queue_id);
        if ((msgsnd(clients[id]->queue_id, to_send, sizeof(*to_send) - sizeof(long), MSG_NOERROR)) == -1) {
            perror("Error while sending message to client");
        } else {
            printf("%d: Message sent\n", getpid());
        }
    }
}

void handle_echo(message *pMessage) {
    printf("%d: before sending\n", getpid());
    send_message(pMessage);
}

void handle_caps(message *pMessage) {
    for (int i = 0; pMessage->message[i] != '\0'; ++i) {
        printf("%d: uppercasing '%c'\n", getpid(), pMessage->message[i]);
        pMessage->message[i] = (char) toupper(pMessage->message[i]);
        printf("%d: uppercased '%c'\n", getpid(), pMessage->message[i]);
    }

    printf("%d: before sending\n", getpid());
    send_message(pMessage);
}

void handle_time(message *pMessage) {
    time_t now = time(NULL);
    message *response = create_message(pMessage->client, ctime(&now));
    send_message(response);
    free(response);
}

void handle_exit(message *pMessage) {
    printf("%d: Server received time request from PID %d\n", getpid(), pMessage->client);
    handle_pending_requests();
    msgctl(queue_id, IPC_RMID, NULL);
    exit(EXIT_SUCCESS);
}

void handle_pending_requests() {
    message *msg;
    printf("%d: Server going to sleep to accumulate pending requests\n", getpid());
    printf("%d: Server woken up\n", getpid());

    while ((msg = receive_message()) != NULL) {
        process_message(msg);
        free(msg);
    }
}

message *receive_message() {
    message *msg = calloc(1, sizeof(*msg));

    printf("%d: Server is receiving message\n", getpid());
    if ((msgrcv(queue_id, msg, sizeof(*msg) - sizeof(long), 0, MSG_NOERROR | IPC_NOWAIT)) == -1) {
        if (errno != ENOMSG) {
            perror("Error while receiving message");
        }
        return NULL;
    };
    printf("%d: Server received message \"%s\" with type %ld and PID %d\n", getpid(), msg->message, msg->message_type, msg->client);
    return msg;
}
