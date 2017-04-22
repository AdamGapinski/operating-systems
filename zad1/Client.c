#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <wait.h>
#include "Common.h"

void print_options() ;

int read_option() ;

void process_option(int option) ;

void request_echo() ;

void request_caps() ;

void request_time() ;

void request_exit() ;

void send_to_server(int queue_id) ;

int receive_identity(int queue_id) ;

int create_queue() ;

void echo_option() ;

void caps_option() ;

void time_option() ;

char * receive_response() ;

int main() {
    printf("%d: Client started\n", getpid());
    int queue_id = create_queue();
    send_to_server(queue_id);
    int client_identity = receive_identity(queue_id);

    printf("%d: Client is entering interactive mode to send requests\n", getpid());
    while (1) {
        print_options();
        process_option(read_option());
    }

    return 0;
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

void send_to_server(int queue_id) {
    printf("%d: Client is sending key %d to server\n", getpid(), queue_id);
}

int receive_identity(int queue_id) {
    printf("%d: Client is waiting for ID from server\n", getpid());
    sleep(1);
    return 0;
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
