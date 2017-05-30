#ifndef THREADSSYNCHRONIZATION_UTILS_H
#define THREADSSYNCHRONIZATION_UTILS_H
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif

#define MAX_CLIENTS 22
#define CLIENT_MAX_NAME 64
#define OPERATION_REQ_MSG 0
#define OPERATION_RES_MSG 1
#define NAME_REQ_MSG 2
#define PING_REQUEST 3
#define PING_RESPONSE 4
#define REGISTERED_RES_MSG 5
#define NOT_REGISTERED_RES_MSG 6

typedef struct Message {
    short type;
    short length;
} Message;

char *parseTextArg(int argc, char **argv, int arg_num, char *des);
int parseUnsignedIntArg(int argc, char **argv, int arg_num, char *des);
void setSigIntHandler(void (*handler)(int));
long get_thread_id();
int send_message(int socket_fd, Message *message, void *data);
void *receive_message(int socket_fd, Message *message);
void make_log(char *logArg, int var);

#endif //THREADSSYNCHRONIZATION_UTILS_H
