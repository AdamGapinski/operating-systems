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

pthread_mutex_t logger_write_mutex = PTHREAD_MUTEX_INITIALIZER;

char *filename = "logs";
FILE *logfile = NULL;

void data_logging(int data_type, void *data, int sending);

void *handle_recv_res(int received, int data_len, void *data, int data_type) ;

char *parse_text_arg(int argc, char **argv, int arg_num, char *des) {
    if (argc <= arg_num) {
        fprintf(stderr, "Error: Argument %d. - %s not specified\n", arg_num, des);
        exit(EXIT_FAILURE);
    }
    return argv[arg_num];
}

int parse_unsigned_int_arg(int argc, char **argv, int arg_num, char *des) {
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

void set_sig_int_handler(void (*handler)(int)) {
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

int send_message_to(int socket_fd, Message *message, void *data, struct sockaddr* adr, socklen_t adr_len) {
    size_t msg_bytes = sizeof(*message) + message->length;
    void *msg_structure_pointer = calloc(msg_bytes, 1);
    Message *msg_data = (Message *) msg_structure_pointer;
    msg_data->type = message->type;
    msg_data->length = message->length;
    void *data_pointer = ((char *) msg_data) + sizeof(*msg_data);
    memcpy(data_pointer, data, (size_t) message->length);

    data_logging(message->type, data_pointer, 1);

    msg_data->type = htobe16(message->type);
    msg_data->length = htobe16(message->length);
    if (sendto(socket_fd, msg_structure_pointer, msg_bytes, MSG_DONTWAIT, adr, adr_len) != msg_bytes) {
        make_log("sending error", 0);
        return -1;
    }
    free(msg_structure_pointer);
    return 0;
}

int send_message(int socket_fd, Message *message, void *data) {
    size_t msg_bytes = sizeof(*message) + message->length;
    void *msg_structure_pointer = calloc(msg_bytes, 1);
    Message *msg_data = (Message *) msg_structure_pointer;
    msg_data->type = message->type;
    msg_data->length = message->length;
    void *data_pointer = ((char *) msg_data) + sizeof(*msg_data);
    memcpy(data_pointer, data, (size_t) message->length);

    data_logging(message->type, data_pointer, 1);

    msg_data->type = htobe16(message->type);
    msg_data->length = htobe16(message->length);
    if (send(socket_fd, msg_structure_pointer, msg_bytes, 0) != msg_bytes) {
        make_log("sending error", 0);
        return -1;
    }
    free(msg_structure_pointer);
    return 0;
}

void data_logging(int data_type, void *data, int sending) {
    char *activity = sending ? "SENDING" : "RECEIVING";
    make_log(activity, 0);

    char *text;
    Operation *operation;
    switch (data_type) {
        case NAME_REQ_MSG:
            text = data;
            char buf[CLIENT_MAX_NAME + 10];
            sprintf(buf, "name %s", text);
            make_log(buf, 0);
            break;
        case OPERATION_REQ_MSG:
        case OPERATION_RES_MSG:
            operation = data;
            make_log("operation: operation id %d", operation->operation_id);
            make_log("operation: operation type %d", operation->operation_type);
            make_log("operation: first argument %d", (int) operation->first_argument);
            make_log("operation: second argument %d", (int) operation->second_argument);
            break;
        case PING_REQUEST:
            make_log("ping: request message", 0);
            break;
        case PING_RESPONSE:
            make_log("ping: response message", 0);
            break;
        case REGISTERED_RES_MSG:
            make_log("registered: response message", 0);
            break;
        case NOT_REGISTERED_RES_MSG:
            make_log("not registered: response message", 0);
            break;
        case UNREGISTER_REQ_MSG:
            make_log("unregister: request message", 0);
            break;
        default:
            make_log("unsupported data logging type: %d", data_type);
    }
}

void *receive_message_from(int socket_fd, Message *message, struct sockaddr* adr, socklen_t *adr_len) {
    size_t msg_header_size = sizeof(*message);
    int received = (int) recvfrom(socket_fd, message, msg_header_size, MSG_PEEK | MSG_DONTWAIT, adr, adr_len);
    if (handle_recv_res(received, (int) msg_header_size, message, -1) == NULL) {
        return NULL;
    }
    message->type = be16toh(message->type);
    message->length = be16toh(message->length);

    void *data;
    void *data_buf;
    switch(message->type) {
        case NAME_REQ_MSG:
        case OPERATION_REQ_MSG:
        case OPERATION_RES_MSG:
            data_buf = calloc(msg_header_size + message->length, 1);
            data = calloc((size_t) message->length, 1);
            received = (int) recvfrom(socket_fd, data_buf, msg_header_size + message->length, MSG_DONTWAIT, adr, adr_len);
            memcpy(data, ((char *) data_buf) + msg_header_size, (size_t) message->length);
            free(data_buf);
            return handle_recv_res(received, (int) (msg_header_size + message->length), data, message->type);
        case PING_REQUEST:
        case PING_RESPONSE:
        case REGISTERED_RES_MSG:
        case NOT_REGISTERED_RES_MSG:
        case UNREGISTER_REQ_MSG:
            data = calloc(1, 1);
            //receive message header from socket receiving queue
            recvfrom(socket_fd, NULL, msg_header_size, MSG_DONTWAIT, adr, adr_len);
            return handle_recv_res(0, message->length, data, message->type);
        default:
            make_log("Receiving error unsupported message type", 0);
            //receive message header from socket receiving queue
            recvfrom(socket_fd, NULL, msg_header_size, MSG_DONTWAIT, adr, adr_len);
            return NULL;
    }
}

void *receive_message(int socket_fd, Message *message) {
    size_t msg_header_size = sizeof(*message);
    int received = (int) recv(socket_fd, message, msg_header_size, MSG_PEEK);
    if (handle_recv_res(received, (int) msg_header_size, message, -1) == NULL) {
        return NULL;
    }
    message->type = be16toh(message->type);
    message->length = be16toh(message->length);

    void *data;
    void *data_buf;
    switch(message->type) {
        case NAME_REQ_MSG:
        case OPERATION_REQ_MSG:
        case OPERATION_RES_MSG:
            data_buf = calloc(msg_header_size + message->length, 1);
            data = calloc((size_t) message->length, 1);
            received = (int) recv(socket_fd, data_buf, msg_header_size + message->length, 0);
            memcpy(data, ((char *) data_buf) + msg_header_size, (size_t) message->length);
            free(data_buf);
            return handle_recv_res(received, (int) (msg_header_size + message->length), data, message->type);
        case PING_REQUEST:
        case PING_RESPONSE:
        case REGISTERED_RES_MSG:
        case NOT_REGISTERED_RES_MSG:
        case UNREGISTER_REQ_MSG:
            data = calloc(1, 1);
            //receive message header from socket receiving queue
            recv(socket_fd, NULL, msg_header_size, 0);
            return handle_recv_res(0, message->length, data, message->type);
        default:
            make_log("Receiving error unsupported message type", 0);
            //receive message header from socket receiving queue
            recv(socket_fd, NULL, msg_header_size, 0);
            return NULL;
    }
}

void *handle_recv_res(int received, int data_len, void *data, int data_type) {
    if (received == data_len) {
        if (data_type != -1) {
            data_logging(data_type, data, 0);
        }
        return data;
    } else if (received == 0) {
        make_log("Receiving error - connection closed", 0);
        return NULL;
    }
    else {
        make_log("Receiving error", 0);
        return NULL;
    }
}

char log_buf[512];
void make_log(char *logArg, int var) {
    pthread_mutex_lock(&logger_write_mutex);
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (logfile == NULL) {
        if ((logfile = fopen(filename, "a")) == NULL) {
            perror("Logger: open file error");
        }
    }
    if (logfile != NULL) {
        sprintf(log_buf, logArg, var);
        fprintf(logfile, "%ld TID %ld s: %s\n", get_thread_id(), ts.tv_sec, log_buf);
    }
    fflush(logfile);
    pthread_mutex_unlock(&logger_write_mutex);
}
