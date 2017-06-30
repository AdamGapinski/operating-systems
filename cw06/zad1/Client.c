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

message *wait_for_message();

void clean_at_exit() ;

long receive_identity() ;

void wait_for_key() ;

int get_server_queue_id();

int queue_id = -1;
int server_qid = -1;
long client_identity = 0;

int main() {
    printf("%d: Client started\n", getpid());
    atexit(clean_at_exit);

    int key = create_queue();
    register_with_key(key);
    client_identity = receive_identity();

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
    srand((unsigned int) (time(NULL) ^ (getpid() << 16)));
    int key;
    do {
        /*
         * Key seed is randomized from the set of key seeds defined in common.h header
         * */
        if ((key = ftok(getenv("HOME"), KEY_SEEDS[rand() % KEY_SEEDS_SIZE])) == -1) {
            perror("Error while generating queue key");
        }

        errno = 0;
        /*
         * Checking if there exists another message queue with the same key
         * and if it exists (errno set to EEXITS, then try to generate key once more
         * */
        if ((queue_id = msgget(key, IPC_CREAT | IPC_EXCL | 0666)) == -1 && errno != EEXIST) {
            perror("Error while creating client queue");
            exit(EXIT_FAILURE);
        }
    } while (errno == EEXIST);
    errno = 0;
    return key;
}

void register_with_key(int key) {
    char *key_buf = calloc(MSG_MAX_SIZE, sizeof(*key_buf));
    sprintf(key_buf, "%d", key);
    /*
     * key value generated from ftok is sent to server
     * */
    send_to_server(key_buf, REGISTER);
    free(key_buf);
}

void send_to_server(char *text, long message_type) {
    message *request = malloc(sizeof(*request));
    request->client = getpid();
    request->message_type = message_type;
    strncpy(request->message, text, MSG_MAX_SIZE);

    if (server_qid == -1) {
        server_qid = get_server_queue_id();
    }

    if ((msgsnd(server_qid, request, sizeof(*request) - sizeof(long), 0))) {
        if (errno == EINVAL) {
            fprintf(stderr, "Error while sending request: server not available\n");
        } else {
            perror("Error while sending request");
        }
        exit(EXIT_FAILURE);
    }
    free(request);
}

int get_server_queue_id() {
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
    return server_qid;
}

long receive_identity() {
    message *received = wait_for_message();
    long message_type;
    if (received != NULL) {
        message_type = received->message_type;
        free(received);
    } else {
        exit(EXIT_FAILURE);
    }

    return message_type;
}

message *wait_for_message() {
    message *msg = calloc(1, sizeof(*msg));

    const int SECONDS_TO_WAIT = 5;
    const int SECONDS_TO_MICROSECONDS = (const int) 1e6;
    const int U_INTERVAL = (const int) 1e3;   //time in microseconds between calls to msgrcv, 10000 is 100 ms
    for (int i = 0; i < SECONDS_TO_WAIT * SECONDS_TO_MICROSECONDS / U_INTERVAL; ++i) {
        if (msgrcv(queue_id, msg, sizeof(*msg) - sizeof(long), client_identity, MSG_NOERROR | IPC_NOWAIT) == -1) {
            usleep((__useconds_t) U_INTERVAL);
        } else {
            return msg;
        }
    }
    if ((msgrcv(queue_id, msg, sizeof(msg) - sizeof(long), client_identity, MSG_NOERROR | IPC_NOWAIT)) == -1) {
        perror("Timeout error");
        return NULL;
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
    char *message_to_send = "Test message";
    printf("Wsylana wiadomosc: %s\n", message_to_send);
    send_to_server(message_to_send, ECHO);
    message *response = wait_for_message();
    if (response != NULL) {
        printf("Otrzymana opdpowiedz: %s\n", response->message);
    }
    free(response);
}

void caps_option() {
    char *message_to_send = "Test message";
    printf("Wsylana wiadomosc: %s\n", message_to_send);
    send_to_server(message_to_send, CAPS);
    message *response = wait_for_message();
    if (response != NULL) {
        printf("Otrzymana opdpowiedz: %s\n", response->message);
    }
    free(response);
}

void time_option() {
    send_to_server("", TIME);
    message *response = wait_for_message();
    if (response != NULL) {
        printf("Otrzymana opdpowiedz: %s", response->message);
    }
    free(response);
}

void exit_option() {
    send_to_server("", EXIT);
}
