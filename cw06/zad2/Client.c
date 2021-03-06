#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include "common.h"

void print_options() ;

int read_option() ;

void process_option(int option) ;

void echo_option() ;

void caps_option() ;

void time_option() ;

void exit_option() ;

message *wait_for_message();

void clean_at_exit() ;

void wait_for_key() ;

char *create_queue() ;

void send_to_server(char *text, char message_type) ;

char receive_identity() ;

mqd_t queue_descriptor = -1;
mqd_t server_queue = -1;
char *queue_name;

int main() {
    printf("%d: Client started\n", getpid());
    atexit(clean_at_exit);

    queue_name = create_queue();
    send_to_server(queue_name, REGISTER);
    receive_identity();

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
    if (queue_descriptor != -1) {
        send_to_server("", CLIENT_EXIT);
        mq_close(queue_descriptor);
        mq_close(server_queue);
        mq_unlink(queue_name);
    }
}
char *create_queue() {
    srand((unsigned int) (time(NULL) ^ (getpid() << 16)));
    char *queue_name;
    do {
        const int queue_name_size = NAME_MAX / 16;
        queue_name = calloc(queue_name_size, sizeof(*queue_name));
        queue_name[0] = '/';
        for (int i = 1 ; i < queue_name_size - 1; ++i) {
            queue_name[i] = (char) ('a' + rand() % 26);
        }
        queue_name[queue_name_size - 1] = '\0';

        struct mq_attr attr;
        attr.mq_maxmsg = MAX_ON_QUEUE;
        attr.mq_msgsize = MAX_MSG_SIZE;
        errno = 0;
        if ((queue_descriptor = mq_open(queue_name, O_RDONLY | O_CREAT | O_EXCL, 0666, &attr)) == -1 && errno != EEXIST) {
            perror("Error while creating client queue");
            exit(EXIT_FAILURE);
        }
    } while (errno == EEXIST);
    errno = 0;
    return queue_name;
}

void send_to_server(char *text, char message_type) {
    message *request = malloc(sizeof(*request));
    request->client = getpid();
    request->message_type = message_type;
    strncpy(request->message, text, MAX_MSG_TEXT_SIZE);

    if (server_queue == -1 && (server_queue = mq_open(SERVER_QUEUE_NAME, O_WRONLY)) == -1) {
        perror("Error while opening server queue");
    }

    if ((mq_send(server_queue, (char *) request, sizeof(*request), 0)) == -1) {
        perror("Error while sending message to server");
        if (message_type == REGISTER) {
            exit(EXIT_FAILURE);
        }
    }

    free(request);
}

char receive_identity() {
    message *received = wait_for_message();
    char message_type;
    if (received != NULL) {
        message_type = received->message_type;
        free(received);
    } else {
        fprintf(stderr, "Error while registering client\n");
        exit(EXIT_FAILURE);
    }

    return message_type;
}

message *wait_for_message() {
    message *msg = calloc(1, sizeof(*msg));

    const int SECONDS_TO_WAIT = 5;
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += SECONDS_TO_WAIT;
    if ((mq_timedreceive(queue_descriptor, (char *) msg, sizeof(*msg), 0, &timeout)) == -1) {
        perror("Error while receiving message");
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
