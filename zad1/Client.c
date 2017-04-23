#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "common.h"

void print_options() ;

int read_option() ;

void process_option(int option) ;

int create_queue() ;

void echo_option() ;

void caps_option() ;

void time_option() ;

void exit_option() ;

void send_to_server(char *text, long message_type) ;

void register_with_key(int key);

message *receive_message();

void clean_at_exit() ;

long receive_identity() ;

void wait_for_key() ;

int queue_id = -1;
long client_identity = 0;

int main() {
    printf("%d: Client started\n", getpid());
    atexit(clean_at_exit);

    int key = create_queue();
    register_with_key(key);
    client_identity = receive_identity();

    printf("%d: Client is entering interactive mode to send requests\n", getpid());
    while (1) {
        print_options();
        process_option(read_option());
        wait_for_key();
    }
}

void wait_for_key() {
    printf("press ENTER key to continue\n");
    //flushing stdin to wait for ENTER
    while (getchar() != '\n');
    getchar();
}

void clean_at_exit() {
    if (queue_id != -1) {
        msgctl(queue_id, IPC_RMID, NULL);
    }
}

int create_queue() {
    printf("%d: Client is creating message queue\n", getpid());

    srand((unsigned int) (time(NULL) ^ (getpid() << 16)));
    int key;
    do {
        int rand_i = rand() % KEY_SEEDS_SIZE;
        printf("rand %d\n", rand_i);

        if ((key = ftok(getenv("HOME"), KEY_SEEDS[rand_i])) == -1) {
            perror("Error while generating queue key");
        }

        errno = 0;
        if ((queue_id = msgget(key, IPC_CREAT | IPC_EXCL | 0666)) == -1 && errno != EEXIST) {
            perror("Error while creating client queue");
            exit(EXIT_FAILURE);
        }
    } while (errno == EEXIST);
    errno = 0;

    printf("%d: Client created message queue with key %d and id %d\n", getpid(), key, queue_id);
    return key;
}

void register_with_key(int key) {
    printf("%d: Client is registering to server with queue id %d\n", getpid(), key);
    char *queue_buf = calloc(MSG_MAX_SIZE, sizeof(*queue_buf));
    sprintf(queue_buf, "%d", key);
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

    printf("%d: Client is sending message: \"%s\" to queue with key: %d and id: %d\n", getpid(), request->message, key, server_qid);
    if ((msgsnd(server_qid, request, sizeof(*request) - sizeof(long), 0))) {
        perror("Error while sending register request");
        exit(EXIT_FAILURE);
    }
}

long receive_identity() {
    printf("%d: Client is waiting for ID from server\n", getpid());
    message *received = receive_message();
    if (received->message_type == 0) {
        fprintf(stderr, "Error while registering client");
        exit(EXIT_FAILURE);
    }
    printf("%d: Client registered with identity %ld\n", getpid(), received->message_type);
    return received->message_type;
}

message *receive_message() {
    message *msg = calloc(1, sizeof(*msg));

    printf("%d: Client is waiting for message\n", getpid());

    const int SECONDS_TO_WAIT = 5;
    const int SECONDS_TO_MICROSECONDS = (const int) 1e6;
    const int U_INTERVAL = (const int) 1e3;   //time in microseconds between calls to msgrcv, 10000 is 100 ms
    for (int i = 0; i < SECONDS_TO_WAIT * SECONDS_TO_MICROSECONDS / U_INTERVAL; ++i) {
        if (msgrcv(queue_id, msg, sizeof(*msg) - sizeof(long), client_identity, MSG_NOERROR | IPC_NOWAIT) == -1) {
            usleep((__useconds_t) U_INTERVAL);
        } else {
            printf("%d: Client received message \"%s\" type %ld PID %d\n", getpid(), msg->message, msg->message_type, msg->client);
            return msg;
        }
    }

    if ((msgrcv(queue_id, msg, sizeof(msg) - sizeof(long), client_identity, MSG_NOERROR | IPC_NOWAIT)) == -1) {
        perror("Error while receiving message");
    }

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
    int result = 0;
    scanf("%d", &result);
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
            exit_option();
            break;
        case 5:
            exit(EXIT_SUCCESS);
        default:
            printf("Nierozpoznana opcja\n");
    }
}

void echo_option() {
    printf("%d: Client is requesting echo\n", getpid());
    send_to_server("Test message", ECHO);
    printf("%d: Client is waiting for response\n", getpid());
    char *response = receive_message()->message;
    printf("Otrzymana opdpowiedz: %s\n", response);
}

void caps_option() {
    printf("%d: Client is requesting caps\n", getpid());
    send_to_server("Test message", CAPS);
    printf("%d: Client is waiting for response\n", getpid());
    char *response = receive_message()->message;
    printf("Otrzymana opdpowiedz: %s\n", response);
}

void time_option() {
    printf("%d: Client is requesting time\n", getpid());
    send_to_server("", TIME);
    printf("%d: Client is waiting for response\n", getpid());
    char *response = receive_message()->message;
    printf("Otrzymana opdpowiedz: %s", response);
}

void exit_option() {
    printf("%d: Client is requesting exit\n", getpid());
    send_to_server("", EXIT);
    printf("%d: Exit requested\n", getpid());
}
