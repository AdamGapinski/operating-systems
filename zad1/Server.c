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

void handle_pending_requests();

message *receive_message() ;

void handle_exit() ;

typedef struct client {
    pid_t process_id;
    int queue_id;
} client;

client *clients[MAX_CLIENTS + 1];
//skipping 0 index to avoid sending message with 0 message type
int client_index = 1;

int queue_id = 0;

int main() {
    printf("%d: Server started\n", getpid());
    queue_id = create_queue();
    while(1) {
        handle_request();
    }
}

int create_queue() {
    int queue_id;
    if ((queue_id = msgget(ftok(getenv("HOME"), PROJ_ID), IPC_CREAT | 0666)) == -1) {
        perror("Error while creating server queue");
        exit(EXIT_FAILURE);
    }
    return queue_id;
}

void handle_request() {
    message *msg = wait_for_message();
    if (msg != NULL) {
        process_message(msg);
        free(msg);
    }
}

message *wait_for_message() {
    message *msg = calloc(1, sizeof(*msg));
    if ((msgrcv(queue_id, msg, sizeof(*msg) - sizeof(long), 0, MSG_NOERROR)) == -1) {
        perror("Error while receiving message");
        return NULL;
    } else {
        return msg;
    }
}

void process_message(message *msg) {
    switch (msg->message_type) {
        case REGISTER:
            handle_register(msg);
            break;
        case ECHO:
            handle_echo(msg);
            break;
        case CAPS:
            handle_caps(msg);
            break;
        case TIME:
            handle_time(msg);
            break;
        case EXIT:
            handle_exit();
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

    /*opening client queue and getting it's identifier*/
    int key;
    sscanf(msg->message, "%d", &key);
    if ((to_register->queue_id = msgget(key, 0666)) == -1) {
        perror("Error while opening client queue");
    };

    clients[client_index] = to_register;
    ++client_index;

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
        if ((msgsnd(clients[id]->queue_id, to_send, sizeof(*to_send) - sizeof(long), MSG_NOERROR)) == -1) {
            perror("Error while sending message to client");
        }
    }
}

void handle_echo(message *pMessage) {
    send_message(pMessage);
}

void handle_caps(message *pMessage) {
    for (int i = 0; pMessage->message[i] != '\0'; ++i) {
        pMessage->message[i] = (char) toupper(pMessage->message[i]);
    }

    send_message(pMessage);
}

void handle_time(message *pMessage) {
    time_t now = time(NULL);
    message *response = create_message(pMessage->client, ctime(&now));
    send_message(response);
    free(response);
}

void handle_exit() {
    handle_pending_requests();
    msgctl(queue_id, IPC_RMID, NULL);
    exit(EXIT_SUCCESS);
}

void handle_pending_requests() {
    message *msg;
    while ((msg = receive_message()) != NULL) {
        process_message(msg);
        free(msg);
    }
}

message *receive_message() {
    message *msg = calloc(1, sizeof(*msg));

    if ((msgrcv(queue_id, msg, sizeof(*msg) - sizeof(long), 0, MSG_NOERROR | IPC_NOWAIT)) == -1) {
        if (errno != ENOMSG) {
            perror("Error while receiving message");
        }
        return NULL;
    };
    return msg;
}
