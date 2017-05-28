#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syscall.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include "include/utils.h"

pthread_mutex_t logger_read_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t logger_write_mutex = PTHREAD_MUTEX_INITIALIZER;

char *filename = "logs";
char *logMsg = NULL;
pthread_t loggerThread;
int logThreadStarted = 0;

void createLoggerThread();

void handle_recv_res(int received) ;

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

void send_message(int socket_fd, Message *message, void *data) {
    size_t msg_bytes = sizeof(message->type) + sizeof(message->length) + message->length;
    void *msg_structure_pointer = calloc(msg_bytes, sizeof(char));
    Message *msg_data = (Message *) msg_structure_pointer;
    msg_data->type = message->type;
    msg_data->length = message->length;
    void *data_pointer = ((char *) msg_data) + sizeof(*msg_data);
    memcpy(data_pointer, data, (size_t) message->length);

    make_log("sending type %d", message->type);
    make_log("sending length %d", message->length);
    make_log("sending message length %d", (int) msg_bytes);
    make_log("sending message", 0);
    make_log(data_pointer, 0);

    msg_data->type = htobe16(message->type);
    msg_data->length = htobe16(message->length);
    ssize_t send_result;
    if ((send_result = send(socket_fd, msg_structure_pointer, msg_bytes, 0)) != msg_bytes) {
        make_log("client: sending error", 0);
        exit(EXIT_FAILURE);
    }
    free(msg_structure_pointer);
}

void *receive_message(int socket_fd, Message *message) {
    int received = (int) recv(socket_fd, message, sizeof(*message), 0);
    message->type = be16toh(message->type);
    message->length = be16toh(message->length);
    handle_recv_res(received);
    make_log("received type %d", message->type);
    make_log("received length %d", message->length);

    size_t msg_data_bytes;
    switch(message->type) {
        case NAME_REQ_MSG:
        case NAME_RES_MSG:
            msg_data_bytes = message->length * sizeof(char);
            char *data = calloc(msg_data_bytes, sizeof(*data));
            received = (int) recv(socket_fd, data, msg_data_bytes, 0);
            handle_recv_res(received);
            return data;
            break;
        case OPERATION_RES_MSG:
            break;
        default:
            make_log("unsupported opperation type %d", message->type);
            break;
    }
    return NULL;
}

void handle_recv_res(int received) {
    if (received == 0) {
        make_log("client closed connection", 0);
    } else if (received == -1) {
        perror("error recv");
    } else {
        make_log("received %d bytes", received);
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
