#ifndef THREADSSYNCHRONIZATION_UTILS_H
#define THREADSSYNCHRONIZATION_UTILS_H
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define CLIENT_MAX_NAME 64
#define OPERATION_REQ_MSG 0
#define OPERATION_RES_MSG 1
#define NAME_REQ_MSG 2
#define PING_REQUEST 3
#define PING_RESPONSE 4
#define REGISTERED_RES_MSG 5
#define NOT_REGISTERED_RES_MSG 6
#define UNREGISTER_REQ_MSG 7

typedef struct Message {
    short type;
    short length;
} Message;

char *parse_text_arg(int argc, char **argv, int arg_num, char *des);
int parse_unsigned_int_arg(int argc, char **argv, int arg_num, char *des);
void set_sig_int_handler(void (*handler)(int));
long get_thread_id();
int send_message(int socket_fd, Message *message, void *data);
void *receive_message(int socket_fd, Message *message);
void make_log(char *logArg, int var);

#endif //THREADSSYNCHRONIZATION_UTILS_H
