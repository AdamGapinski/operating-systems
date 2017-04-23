#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
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

message *receive_message(mqd_t non_block_queue_desc) ;

void clean_at_exit() ;

void handle_client_exit(message *pMessage) ;

int find_free_slot();

typedef struct client {
    pid_t process_id;
    char id;
    mqd_t queue_descriptor;
} client;

client *clients[MAX_CLIENTS + 1];
//skipping 0 index to avoid sending message with 0 message type
int client_index = 1;

mqd_t queue_descriptor = -1;

int main() {
    printf("%d: Server started\n", getpid());
    atexit(clean_at_exit);

    atexit(clean_at_exit);
    queue_descriptor = create_queue();
    printf("%d: Server is ready to handle requests\n", getpid());

    while(1) {
        handle_request();
    }
}

void clean_at_exit() {
    if (queue_descriptor != -1) {
        for (int i = 1 ; i < client_index; ++i) {
            mq_close(clients[i]->queue_descriptor);
        }
        mq_close(queue_descriptor);
        mq_unlink(SERVER_QUEUE_NAME);
    }
}

int create_queue() {
    printf("%d: Server is creating message queue\n", getpid());

    mqd_t queue_descriptor;

    struct mq_attr attr;
    attr.mq_maxmsg = MAX_ON_QUEUE;
    attr.mq_msgsize = MAX_MSG_SIZE;
    if ((queue_descriptor = mq_open(SERVER_QUEUE_NAME, O_RDONLY | O_CREAT, 0666, &attr)) == -1) {
        perror("Error while creating server queue");
        exit(EXIT_FAILURE);
    }
    printf("%d: Server created message queue with descriptor %d\n", getpid(), queue_descriptor);
    return queue_descriptor;
}

void handle_request() {
    printf("%d: Server is waiting for request\n", getpid());
    message *msg = wait_for_message();
    if (msg != NULL) {
        process_message(msg);
        free(msg);
    }
    printf("%d: Server ended handling request\n", getpid());
}

message *wait_for_message() {
    message *msg = calloc(1, sizeof(*msg));

    printf("%d: Server is waiting for message\n", getpid());
    if ((mq_receive(queue_descriptor, (char *) msg, sizeof(*msg), 0)) == -1) {
        perror("Error while receiving message");
        return NULL;
    } else {
        printf("%d: Server received message \"%s\" with type %d and PID %d\n", getpid(), msg->message, msg->message_type, msg->client);
    }
    return msg;
}

void process_message(message *msg) {
    switch (msg->message_type) {
        case REGISTER:
            printf("%d: Server is registering client\n", getpid());
            handle_register(msg);
            printf("%d: Server processed registering request\n", getpid());
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
        case CLIENT_EXIT:
            printf("%d: Server is processing %s request\n", getpid(), "client exit");
            handle_client_exit(msg);
            printf("%d: Server processed %s request\n", getpid(), "client exit");
            break;
        default:
            printf("%d: Unrecognized request\n", getpid());
    }
}

void handle_register(message *msg) {
    client *to_register = malloc(sizeof(*to_register));
    to_register->process_id = msg->client;

    /*opening client queue and getting it's identifier*/
    char *client_queue_name = calloc(NAME_MAX, sizeof(*client_queue_name));
    sscanf(msg->message, "%s", client_queue_name);
    printf("%d: opening client queue with name %s\n", getpid(), client_queue_name);
    if ((to_register->queue_descriptor = mq_open(client_queue_name, O_WRONLY)) == -1) {
        perror("Error while opening client queue");
    };
    free(client_queue_name);

    int index;
    if ((index = find_free_slot()) != -1) {
        clients[index] = to_register;
        to_register->id = (char) index;
        printf("%d: Client registered at %d id\n", getpid(), index);
        printf("%d: Server is sending back ID %d to client PID: %d\n", getpid(), to_register->id, to_register->process_id);
        message *response = create_message(to_register->process_id, "");
        send_message(response);
        free(response);
    } else {
        printf("%d: Server could not register client, because no free slots available\n", getpid());
    }
}

int find_free_slot() {
    int id;
    for (id = 1 ; id < client_index; ++id) {
        if (clients[id]->process_id == -1) {
            free(clients[id]);
            return id;
        }
    }

    /*
     * clients array is indexed from 1, to skip 0 index = id = message type
     * */
    if (client_index <= MAX_CLIENTS) {
        int tmp = client_index;
        ++client_index;
        return tmp;
    }
    return -1;
}

message *create_message(pid_t client_pid, char *text) {
    message *result = malloc(sizeof(*result));
    strncpy(result->message, text, MAX_MSG_TEXT_SIZE);
    result->client = client_pid;
    return result;
}

int find_client_id(int pid) {
    int id;
    for (id = 1 ; id < client_index; ++id) {
        if (clients[id]->process_id == pid) {
            return id;
        }
    }
    return -1;
}

void send_message(message *to_send) {
    int id = find_client_id(to_send->client);

    if (id == -1) {
        fprintf(stderr, "Error while sending message to client: Client with PID %d is not registered\n", to_send->client);
    } else {
        to_send->message_type = (char) id;
        printf("%d: Server is sending message \"%s\" to PID %d with queue descriptor: %d\n", getpid(), to_send->message, to_send->client, clients[id]->queue_descriptor);
        if ((mq_send(clients[id]->queue_descriptor, (char *) to_send, sizeof(*to_send), 0)) == -1) {
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
    printf("%d: Server received exit request from PID %d\n", getpid(), pMessage->client);
    handle_pending_requests();
    exit(EXIT_SUCCESS);
}

void handle_client_exit(message *pMessage) {
    printf("%d: Server received client exit request from PID %d\n", getpid(), pMessage->client);
    int id = find_client_id(pMessage->client);

    if (id != -1) {
        mq_close(clients[id]->queue_descriptor);
        clients[id]->process_id = -1;

        /*
         * If this was the last client in clients array, we have to subtract client_index by 1, because client
         * */
        if (client_index - 1 == id) {
            --client_index;
        }
    }
}

void handle_pending_requests() {
    message *msg;

    mqd_t non_block_queue_desc = mq_open(SERVER_QUEUE_NAME, O_RDONLY | O_NONBLOCK);
    while ((msg = receive_message(non_block_queue_desc)) != NULL) {
        process_message(msg);
        free(msg);
    }
    mq_close(non_block_queue_desc);
}

message *receive_message(mqd_t non_block_queue_desc) {
    message *msg = calloc(1, sizeof(*msg));

    printf("%d: Server is waiting for message\n", getpid());
    if ((mq_receive(non_block_queue_desc, (char *) msg, sizeof(*msg), 0)) == -1) {
        if (errno != EAGAIN) {
            fprintf(stderr, "Error while receiving message %s\n", strerror(errno));
        } else {
            printf("%d: No messages\n", getpid());
        }
        return NULL;
    } else {
        printf("%d: Server received message \"%s\" with type %d and PID %d\n", getpid(), msg->message, msg->message_type, msg->client);
    }
    return msg;
}
