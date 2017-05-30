#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syscall.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include "include/utils.h"
#include "include/queue.h"

pthread_mutex_t logger_read_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t logger_write_mutex = PTHREAD_MUTEX_INITIALIZER;

char *filename = "logs";
char *logMsg = NULL;
pthread_t loggerThread;
int logThreadStarted = 0;

void createLoggerThread();

void data_logging(int data_type, void *data);

void *handle_recv_res(int received, int data_len, void *data, int data_type) ;

char *parseTextArg(int argc, char **argv, int arg_num, char *des) {
    if (argc <= arg_num) {
        fprintf(stderr, "Error: Argument %d. - %s not specified\n", arg_num, des);
        exit(EXIT_FAILURE);
    }
    return argv[arg_num];
}

int parseUnsignedIntArg(int argc, char **argv, int arg_num, char *des) {
    if (argc <= arg_num) {
        fprintf(stderr, "Error: Argument %d. - %s not specified\n", arg_num, des);
        exit(EXIT_FAILURE);
    }
    char *endPtr;
    int result = (int) strtol(argv[arg_num], &endPtr, 10);
    if (result < 0 || strcmp(endPtr, "") != 0) {
        fprintf(stderr, "Error: Invalid argument %d. - %s\n", arg_num, des);
        exit(EXIT_FAILURE);
    }
    return result;
}

void setSigIntHandler(void (*handler)(int)) {
    struct sigaction sigactionStr;
    sigactionStr.sa_handler = handler;

    if (sigaction(SIGINT, &sigactionStr, NULL) == -1) {
        perror("Setting SIGINT handler error");
        exit(EXIT_FAILURE);
    }
}

long get_thread_id() {
    return syscall(SYS_gettid);
}

int send_message(int socket_fd, Message *message, void *data) {
    size_t msg_bytes = sizeof(*message) + message->length;
    void *msg_structure_pointer = calloc(msg_bytes, 1);
    Message *msg_data = (Message *) msg_structure_pointer;
    msg_data->type = message->type;
    msg_data->length = message->length;
    void *data_pointer = ((char *) msg_data) + sizeof(*msg_data);
    memcpy(data_pointer, data, (size_t) message->length);

    data_logging(message->type, data_pointer);

    msg_data->type = htobe16(message->type);
    msg_data->length = htobe16(message->length);
    if (send(socket_fd, msg_structure_pointer, msg_bytes, 0) != msg_bytes) {
        make_log("sending error", 0);
        perror("sending error");
        return -1;
    }
    free(msg_structure_pointer);
    return 0;
}

void data_logging(int data_type, void *data) {
    char *text;
    Operation *operation;
    make_log("SENDING | RECEIVING", 0);
    switch (data_type) {
        case NAME_REQ_MSG:
            text = data;
            make_log(text, 0);
            break;
        case OPERATION_REQ_MSG:
        case OPERATION_RES_MSG:
            operation = data;
            make_log("operation: first argument %d", (int) operation->first_argument);
            make_log("operation: second argument %d", (int) operation->second_argument);
            make_log("operation: operation %d", operation->operation_type);
            make_log("operation: operation ID %d", operation->operation_id);
            make_log("operation: client_id %d", operation->client_id);
            break;
        case PING_REQUEST:
            make_log("Ping request", 0);
            break;
        case PING_RESPONSE:
            make_log("Ping response", 0);
            break;
        case REGISTERED_RES_MSG:
            make_log("client registered response", 0);
        case NOT_REGISTERED_RES_MSG:
            make_log("client not registered response", 0);
        default:
            make_log("unsupported operation type: %d", data_type);
            break;
    }
}

void *receive_message(int socket_fd, Message *message) {
    int received = (int) recv(socket_fd, message, sizeof(*message), 0);
    if (handle_recv_res(received, sizeof(*message), message, -1) == NULL) {
        return NULL;
    }
    message->type = be16toh(message->type);
    message->length = be16toh(message->length);

    int *data;
    char *text;
    Operation *operation;
    switch(message->type) {
        case NAME_REQ_MSG:
            text = calloc((size_t) message->length, 1);
            received = (int) recv(socket_fd, text, (size_t) message->length, 0);
            return handle_recv_res(received, message->length, text, message->type);
        case OPERATION_REQ_MSG:
        case OPERATION_RES_MSG:
            operation = calloc((size_t) message->length, 1);
            received = (int) recv(socket_fd, operation, (size_t) message->length, 0);
            return handle_recv_res(received, message->length, operation, message->type);
        case PING_REQUEST:
        case PING_RESPONSE:
        case REGISTERED_RES_MSG:
        case NOT_REGISTERED_RES_MSG:
            data = malloc(sizeof(*data));
            return handle_recv_res(0, message->length, data, message->type);
        default:
            make_log("receiving error - invalid data type", 0);
            perror("receiving error - invalid data type");
            return NULL;
    }
}

void *handle_recv_res(int received, int data_len, void *data, int data_type) {
    if (received == data_len) {
        if (data_type != -1) data_logging(data_type, data);
        return data;
    } else if (received == 0) {
        make_log("receiving error - connection closed", 0);
        return NULL;
    }
    else {
        make_log("receiving error", 0);
        return NULL;
    }
}

void *startLoggerThread(void *filename) {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    FILE *logFile;
    if ((logFile = fopen(filename, "a")) == NULL) {
        perror("logger: open file error");
        pthread_exit((void *) 1);
    }
    while (1) {
        pthread_mutex_lock(&logger_read_mutex);
        size_t info_size = sizeof(*logMsg);
        size_t info_length = strlen(logMsg);
        if (fwrite(logMsg, info_size, info_length, logFile) != info_size * info_length) {
            perror("logger: write file error");
            pthread_mutex_unlock(&logger_read_mutex);
            pthread_exit((void *) 1);
        };
        fflush(logFile);
        free(logMsg);
        pthread_mutex_unlock(&logger_write_mutex);
    }
}

void make_log(char *logArg, int var) {
    pthread_mutex_lock(&logger_write_mutex);
    if (logArg == NULL) logArg = "NULL";
    if (logThreadStarted == 0) {
        pthread_mutex_lock(&logger_read_mutex);
        createLoggerThread();
    }
    if (logThreadStarted == 1) {
        logMsg = calloc(strlen(logArg) + 512, sizeof(*logMsg));
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        sprintf(logMsg, "%ld TID %ld s %ld us: %s\n", get_thread_id(), ts.tv_sec, ts.tv_nsec / 1000, logArg);
        sprintf(logMsg, logMsg, var);
    }
    pthread_mutex_unlock(&logger_read_mutex);
}

void createLoggerThread() {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    int errno;
    if ((errno = pthread_create(&loggerThread, &attr, startLoggerThread, filename)) != 0) {
        char errMsgBuf[256];
        sprintf(errMsgBuf, "Error creating logger thread: %s", strerror(errno));
        fprintf(stderr, "%s", errMsgBuf);
    } else {
        logThreadStarted = 1;
    }
    pthread_attr_destroy(&attr);
}
