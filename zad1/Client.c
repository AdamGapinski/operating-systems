#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

void print_options() ;

int read_option() ;

void process_option(int option) ;

void request_echo() ;

void request_caps() ;

void request_time() ;

void request_exit() ;

int create_queue() ;

void echo_option() ;

void caps_option() ;

void time_option() ;

char * receive_response() ;

void send_to_server(char *text, long message_type) ;

void register_with_qid(int queue_id);

message *receive_message(int queue_id) ;

long receive_identity(int queue_id) ;

int main() {
    printf("%d: Client started\n", getpid());
    int queue_id = create_queue();
    register_with_qid(queue_id);
    long client_identity = receive_identity(queue_id);

    printf("%d: Client is entering interactive mode to send requests\n", getpid());
    while (1) {
        print_options();
        process_option(read_option());
    }
}

int create_queue() {
    printf("%d: Client is creating message queue\n", getpid());
    int queue_id;
    if ((queue_id = msgget(IPC_PRIVATE, 0666)) == -1) {
        perror("Error while creating client queue");
        exit(EXIT_FAILURE);
    }
    printf("%d: Client created message queue with id %d\n", getpid(), queue_id);
    return queue_id;
}

void register_with_qid(int queue_id) {
    printf("%d: Client is registering to server with queue id %d\n", getpid(), queue_id);
    char *queue_buf = calloc(MSG_MAX_SIZE, sizeof(*queue_buf));
    sprintf(queue_buf, "%d", queue_id);
    send_to_server(queue_buf, REGISTER);
    free(queue_buf);
}

void send_to_server(char *text, long message_type) {
    printf("%d: Client is sending %s to server\n", getpid(), text);
    //todo deallocate request
    message *request = malloc(sizeof(*request));
    request->client = getpid();
    request->message_type = message_type;
    strncpy(request->message, text, MSG_MAX_SIZE);

    int key;
    if ((key = ftok(getenv("HOME"), PROJ_ID)) == -1) {
        perror("Error while getting server queue key");
        exit(EXIT_FAILURE);
    }

    int server_qid;
    if ((server_qid = msgget(key, 0)) == -1) {
        perror("Error while getting server queue id");
        exit(EXIT_FAILURE);
    }

    printf("%d: Client is sending message: \"%s\" to queue with key: %d and id: %d\n", getpid(), text, key, server_qid);
    if ((msgsnd(server_qid, request, sizeof(request) - sizeof(long), 0))) {
        perror("Error while sending register request");
        exit(EXIT_FAILURE);
    }
}

long receive_identity(int queue_id) {
    printf("%d: Client is waiting for ID from server\n", getpid());
    message *received = receive_message(queue_id);
    if (received->message_type == 0) {
        fprintf(stderr, "Error while registering client");
        exit(EXIT_FAILURE);
    }
    printf("%d: Client registered with identity %ld\n", getpid(), received->message_type);
    return received->message_type;
}

message *receive_message(int queue_id) {
    message *msg = calloc(1, sizeof(*msg));

    printf("%d: Client is waiting for message\n", getpid());

    const int SECONDS_TO_WAIT = 5;
    const int SECONDS_TO_MICROSECONDS = (const int) 1e6;
    const int U_INTERVAL = (const int) 1e4;   //time in microseconds between calls to msgrcv, 10000 is 100 ms
    for (int i = 0; i < SECONDS_TO_WAIT * SECONDS_TO_MICROSECONDS / U_INTERVAL; ++i) {
        msgrcv(queue_id, msg, sizeof(msg) - sizeof(long), 0, MSG_NOERROR | IPC_NOWAIT);
        if (msgrcv(queue_id, msg, sizeof(msg) - sizeof(long), 0, MSG_NOERROR | IPC_NOWAIT) == -1) {
            usleep((__useconds_t) U_INTERVAL);
        }
    }

    if ((msgrcv(queue_id, msg, sizeof(msg) - sizeof(long), 0, MSG_NOERROR | IPC_NOWAIT)) == -1) {
        perror("Error while receiving message");
    };
    printf("%d: Client received message %s\n", getpid(), msg->message);
    return msg;
}

void print_options() {
    printf("Wybierz rodzaj uslugi:\n");
    printf("1. Usluga echa\n");
    printf("2. Usluga wersalikow\n");
    printf("3. Usluga czasu\n");
    printf("4. Nakaz zakonczenia\n");
    printf("5. Zakoncz program\n");
}

int read_option() {
    char *buff = calloc(20, sizeof(*buff));
    int result = 0;
    scanf("%s", buff);
    sscanf(buff, "%d", &result);
    return result;
}

void process_option(int option) {
    switch (option) {
        case 1:
            echo_option();
            break;
        case 2:
            caps_option();
            break;
        case 3:
            time_option();
            break;
        case 4:
            request_exit();
            break;
        case 5:
            exit(EXIT_SUCCESS);
        default:
            printf("Nierozpoznana opcja\n");
    }
}

void echo_option() {
    request_echo();
    char *response = receive_response();
    printf("Otrzymana opdpowiedz: %s\n", response);
}

void caps_option() {
    request_caps();
    char *response = receive_response();
    printf("Otrzymana opdpowiedz: %s\n", response);
}

void time_option() {
    request_time();
    char *response = receive_response();
    printf("Otrzymana opdpowiedz: %s\n", response);
}

char *receive_response() {
    char *response = "";
    printf("%d: Client is waiting for response\n", getpid());
    sleep(1);
    printf("%d: Client received response\n", getpid());
    return response;
}

void request_echo() {
    printf("%d: Client is sending echo request to server\n", getpid());
}

void request_caps() {
    printf("%d: Client is sending caps request to server\n", getpid());
}

void request_time() {
    printf("%d: Client is sending time request to server\n", getpid());
}

void request_exit() {
    printf("%d: Client is sending exit request to server\n", getpid());
}
