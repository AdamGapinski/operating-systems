#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <endian.h>
#include <arpa/inet.h>
#include <limits.h>
#include <signal.h>
#include "include/utils.h"
#include "include/queue.h"

#define UNIX_PATH_MAX 108
#define MAX_CLIENTS 20
#define OPERATIONS_QUEUE_SIZE 10
#define PING_TIME 5
#define PING_TIMEOUT 100000
#define EPOLL_WAIT_TIMEOUT 1000

typedef struct Client {
    char *name;
    struct sockaddr *adr;
    socklen_t adr_len;
    int socket_fd;
    int client_id;
} Client;

void start_threads() ;

void *terminal_thread(void *arg) ;

void *socket_thread(void *arg) ;

void *ping_thread(void *arg) ;

void *sending_thread(void *arg) ;

void enqueue_operation(int option) ;

int read_option() ;

int find_slot() ;

int name_available(char *name);

void to_string(Operation *operation, char *buf) ;

void send_operation(Operation *operation) ;

void handle_request(int socket_fd);

void cancel_threads(int signo) ;

void release_resources() ;

void setup_inet_socket(int epoll_fd) ;

void setup_local_socket(int epoll_fd) ;

int try_register_client(int socket, char *name, struct sockaddr *adr, socklen_t adr_len) ;

void handle_operation_result(void *received_data, int client_id) ;

int find_client_index(int client_id) ;

void receive_responses(int socket, int *responded) ;

void unregister_client(int client_id) ;

pthread_t terminal_pthread;
pthread_t socket_pthread;
pthread_t sending_pthread;
pthread_t ping_pthread;

in_port_t port;
char *path;
int binded_local_socket = 0;
int operation_counter = 0;
int client_counter = 0;

Queue *operations;
pthread_mutex_t operations_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t operations_cond_not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t operations_cond_not_empty = PTHREAD_COND_INITIALIZER;

Client *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex_sending_thread = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t clients_mutex_socket_thread = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t clients_cond_registered = PTHREAD_COND_INITIALIZER;

int local_socket;
int inet_socket;

int main(int argc, char *argv[]) {
    port = (in_port_t) parse_unsigned_int_arg(argc, argv, 1, "UDP port number");
    path = parse_text_arg(argc, argv, 2, "Unix local socket path");
    if (strlen(path) >= UNIX_PATH_MAX) {
        fprintf(stderr, "Argument validation error - socket path too long: %s, max %d\n", path, UNIX_PATH_MAX - 1);
        exit(EXIT_FAILURE);
    }
    atexit(release_resources);
    set_sig_int_handler(cancel_threads);
    operations = init_queue(OPERATIONS_QUEUE_SIZE);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        clients[i] = NULL;
    }
    make_log("Server: started", 0);
    srand((unsigned int) time(NULL));
    start_threads();
    pthread_exit((void *) 0);
}

void release_resources() {
    if (binded_local_socket) {
        unlink(path);
    }
    close(local_socket);
    close(inet_socket);
}

void cancel_threads(int signo) {
    if (!pthread_equal(socket_pthread, pthread_self())) {
        pthread_cancel(socket_pthread);
    }
    if (!pthread_equal(sending_pthread, pthread_self())) {
        pthread_cancel(sending_pthread);
    }
    if (!pthread_equal(terminal_pthread, pthread_self())) {
        pthread_cancel(terminal_pthread);
    }
    if (!pthread_equal(ping_pthread, pthread_self())) {
        pthread_cancel(ping_pthread);
    }
    pthread_exit(NULL);
}

void start_threads() {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&terminal_pthread, &attr, terminal_thread, NULL);
    pthread_create(&socket_pthread, &attr, socket_thread, NULL);
    pthread_create(&sending_pthread, &attr, sending_thread, NULL);
    pthread_create(&ping_pthread, &attr, ping_thread, NULL);
    pthread_attr_destroy(&attr);
}

void *terminal_thread(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    while (1) {
        int option = read_option();
        if (option > 0 && option < 5) {
            enqueue_operation(option);
        } else if (option == 5) {
            printf("exited\n");
            //to cancel other threads
            kill(getpid(), SIGINT);
            break;
        } else {
            printf("unsupported operation\n");
        }
    }
    pthread_exit((void *) 0);
}

int read_option() {
    int option;
    int result;
    do {
        printf("Chose operation:\n");
        printf("1. addition\n");
        printf("2. subtraction\n");
        printf("3. multiplication\n");
        printf("4. division\n");
        printf("5. stop server\n");
        option = 0;
        result = scanf("%d", &option);
        if (result == EOF) {
            fprintf(stderr, "stdin EOF\n");
            exit(EXIT_FAILURE);
        }
        if (result == 0) {
            printf("Invalid input\n");
            while (fgetc(stdin) != '\n');
        };
    } while(result == 0 || result == EOF);
    return option;
}

double read_operation_argument(char *info) {
    double argument;
    double result;
    do {
        printf("%s\n", info);
        result = scanf("%lf", &argument);
        if (result == EOF) {
            fprintf(stderr, "stdin EOF\n");
            exit(EXIT_FAILURE);
        }
        if (result == 0) {
            printf("Invalid input\n");
            while (fgetc(stdin) != '\n');
        };
    } while(result == 0 || result == EOF);
    return argument;
}

void enqueue_operation(int option) {
    double first_arg;
    double second_arg;
    first_arg = read_operation_argument("operation first argument");
    second_arg = read_operation_argument("operation second argument");
    if (option == DIVISION && second_arg == 0) {
        printf("Division by zero not supported\n");
        return;
    }
    ++operation_counter;
    Operation *operation = init_operation(option, first_arg, second_arg, operation_counter);
    pthread_mutex_lock(&operations_mutex);
    while (enqueue(operations, operation) == -1) {
        pthread_cond_wait(&operations_cond_not_full, &operations_mutex);
    };
    pthread_cond_signal(&operations_cond_not_empty);
    char buf[128];
    to_string(operation, buf);
    printf("Operation %d. scheduled: %s\n", operation->operation_id, buf);
    pthread_mutex_unlock(&operations_mutex);
}

void to_string(Operation *operation, char *buf) {
    char sign;
    switch(operation->operation_type) {
        case ADDITION: sign = '+'; break;
        case SUBTRACTION: sign = '-'; break;
        case MULTIPLICATION: sign = '*'; break;
        case DIVISION: sign = '/'; break;
        default: sign = '#';break;
    }
    sprintf(buf, "%lf %c %lf", operation->first_argument, sign, operation->second_argument);
}

int socket_thread_end = 0;
void *socket_thread(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        make_log("Server: error epoll_create", 0);
        perror("Error epoll_create");
        exit(EXIT_FAILURE);
    }
    setup_local_socket(epoll_fd);
    setup_inet_socket(epoll_fd);
    struct epoll_event events[MAX_CLIENTS];
    int waited_fds_num;
    while (!socket_thread_end) {
        pthread_mutex_lock(&clients_mutex_socket_thread);
        if ((waited_fds_num = epoll_wait(epoll_fd, events, MAX_CLIENTS + 2, EPOLL_WAIT_TIMEOUT)) == -1) {
            make_log("Server: error epoll_wait", 0);
            perror("Error epoll_wait");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < waited_fds_num; ++i) {
            int event_socket_fd = events[i].data.fd;
            if (((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP)) &&
                    (events[i].events & EPOLLIN) == 0) {
                make_log("Server: closing connection %d", event_socket_fd);
                close(event_socket_fd);
            } else if (events[i].events & EPOLLIN){
                handle_request(event_socket_fd);
            } else {
                make_log("Server: epoll unexpected event %d", events[i].events);
            }
        }
        pthread_mutex_unlock(&clients_mutex_socket_thread);
        usleep(10);
    }
    pthread_exit((void *) 0);
}

void setup_local_socket(int epoll_fd) {
    if ((local_socket = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
        perror("Error setting socket");
        make_log("Server: error setting unix socket, errno: %d", errno);
        exit(EXIT_FAILURE);
    }
    struct sockaddr_un address;
    memset(&address, 0, sizeof(struct sockaddr_un));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, path, sizeof(address.sun_path) - 1);
    if (bind(local_socket, (struct sockaddr *) &address, sizeof(address)) == -1) {
        perror("Error setting socket");
        make_log("Server: error binding unix socket, errno: %d", errno);
        exit(EXIT_FAILURE);
    }
    binded_local_socket = 1;
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = local_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, local_socket, &event) == -1) {
        perror("Error setting socket");
        make_log("Server: error adding unix socket to epoll, errno: %d", errno);
        exit(EXIT_FAILURE);
    }
}

void setup_inet_socket(int epoll_fd) {
    if ((inet_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Error setting socket");
        make_log("Server: error setting inet socket, errno: %d", errno);
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in address;
    memset(&address, 0, sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_port = htobe16(port);
    struct in_addr adr;
    adr.s_addr = INADDR_ANY;
    address.sin_addr = adr;
    if (bind(inet_socket, (struct sockaddr *) &address, sizeof(address)) == -1) {
        perror("Error setting socket");
        make_log("Server: error binding inet socket, errno: %d", errno);
        exit(EXIT_FAILURE);
    }
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = inet_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, inet_socket, &event) == -1) {
        perror("Error setting socket");
        make_log("Server: error adding inet socket to epoll, errno: %d", errno);
        exit(EXIT_FAILURE);
    }
}

void handle_request(int socket_fd) {
    Message message;
    struct sockaddr *adr = calloc(1, sizeof(*adr));
    socklen_t adr_len;
    void *received_data = receive_message_from(socket_fd, &message, adr, &adr_len);
    if (received_data != NULL) {
        switch (message.type) {
            case NAME_REQ_MSG:
                make_log("Server: received register request", 0);
                try_register_client(socket_fd, received_data, adr, adr_len);
                break;
            case OPERATION_RES_MSG:
                make_log("Server: received operation result request", 0);
                handle_operation_result(received_data, message.client_id);
                free(received_data);
                free(adr);
                break;
            case UNREGISTER_REQ_MSG:
                make_log("Server: received unregister request", 0);
                pthread_mutex_lock(&clients_mutex_sending_thread);
                unregister_client(message.client_id);
                pthread_mutex_unlock(&clients_mutex_sending_thread);
                free(received_data);
                free(adr);
                break;
            default:
                make_log("Server: received unsupported request type %d", message.type);
        }
    } else {
        make_log("Server: handling request receiving message error", 0);
    }
}

int try_register_client(int socket, char *name, struct sockaddr *adr, socklen_t adr_len) {
    Message message;
    int slot_index;
    if ((slot_index = find_slot()) == -1) {
        make_log("Server: no free slot for new client", 0);
        message.length = 0;
        message.type = NOT_REGISTERED_RES_MSG;
        send_message_to(socket, &message, NULL, adr, adr_len);
        free(name);
        free(adr);
        return -1;
    }
    if (name_available(name) == -1) {
        make_log("Server: error client requested name not available", 0);
        message.length = 0;
        message.type = NOT_REGISTERED_RES_MSG;
        send_message_to(socket, &message, NULL, adr, adr_len);
        free(name);
        free(adr);
        return -1;
    }
    ++client_counter;
    message.length = 0;
    message.type = REGISTERED_RES_MSG;
    message.client_id = (unsigned int) client_counter;
    if (send_message_to(socket, &message, NULL, adr, adr_len) == -1) {
        make_log("Server: error sending registered response", 0);
    };

    pthread_mutex_lock(&clients_mutex_sending_thread);
    clients[slot_index] = malloc(sizeof(*clients[slot_index]));
    clients[slot_index]->name = name;
    clients[slot_index]->adr = adr;
    clients[slot_index]->adr_len = adr_len;
    clients[slot_index]->socket_fd = socket;
    clients[slot_index]->client_id = message.client_id;
    pthread_cond_signal(&clients_cond_registered);
    pthread_mutex_unlock(&clients_mutex_sending_thread);
    char buf[CLIENT_MAX_NAME + 40];
    sprintf(buf, "Server: registered client with name %s", name);
    make_log(buf, 0);
    return 0;
}

int name_available(char *name) {
    pthread_mutex_lock(&clients_mutex_sending_thread);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] != NULL && strcmp(clients[i]->name, name) == 0) {
            pthread_mutex_unlock(&clients_mutex_sending_thread);
            return -1;
        }
    }
    pthread_mutex_unlock(&clients_mutex_sending_thread);
    return 0;
}

int find_slot() {
    pthread_mutex_lock(&clients_mutex_sending_thread);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] == NULL) {
            pthread_mutex_unlock(&clients_mutex_sending_thread);
            return i;
        }
    }
    pthread_mutex_unlock(&clients_mutex_sending_thread);
    return -1;
}

void handle_operation_result(void *received_data, int client_id) {
    Operation *result = (Operation *) received_data;
    char buf[128];
    to_string(result, buf);

    int index;
    pthread_mutex_lock(&clients_mutex_sending_thread);
    if ((index = find_client_index(client_id)) != -1) {
        printf("result of %d. %s = %lf from %s\n", result->operation_id, buf,
               result->result, clients[index]->name);
    } else {
        make_log("Server: received operation result %d from unregistered client", (int) result->result);
    }
    pthread_mutex_unlock(&clients_mutex_sending_thread);
}

void unregister_client(int client_id) {
    int index;
    if ((index = find_client_index(client_id)) != -1) {
        char buf[CLIENT_MAX_NAME + 128];
        sprintf(buf, "Server: unregistering %s", clients[index]->name);
        make_log(buf, 0);
        free(clients[index]->adr);
        free(clients[index]->name);
        free(clients[index]);
        clients[index] = NULL;
    }
}

int sending_end = 0;
void *sending_thread(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    Operation *operation;
    while (!sending_end) {
        pthread_mutex_lock(&operations_mutex);
        while ((operation = dequeue(operations)) == NULL) {
            pthread_cond_wait(&operations_cond_not_empty, &operations_mutex);
        }
        pthread_cond_signal(&operations_cond_not_full);
        pthread_mutex_unlock(&operations_mutex);

        //send_operation blocks until the operation is sent
        send_operation(operation);
        make_log("Server: operation sent %d", operation->operation_id);
        operation = NULL;
        free(operation);
    }
    pthread_exit((void *) 0);
}

void send_operation(Operation *operation) {
    int registered[MAX_CLIENTS];
    int sent;
    int registered_iterator;
    do {
        pthread_mutex_lock(&clients_mutex_sending_thread);
        registered_iterator = 0;
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i] != NULL) {
                registered[registered_iterator] = i;
                ++registered_iterator;
            }
        }
        if (registered_iterator != 0) {
            int rand_registered = rand() % registered_iterator;
            struct sockaddr* adr = clients[registered[rand_registered]]->adr;
            socklen_t adr_len = clients[registered[rand_registered]]->adr_len;
            int socket_fd = clients[registered[rand_registered]]->socket_fd;
            Message message;
            message.length = sizeof(*operation);
            message.type = OPERATION_REQ_MSG;
            if (send_message_to(socket_fd, &message, operation, adr, adr_len) == 0) {
                sent = 1;
            } else {
                make_log("Error: server sending operation %d to client", operation->operation_id);
                sleep(1);
                sent = 0;
            }
        } else {
            make_log("Server: waiting for registered clients", 0);
            pthread_cond_wait(&clients_cond_registered, &clients_mutex_sending_thread);
            sent = 0;
        }
        pthread_mutex_unlock(&clients_mutex_sending_thread);
    } while (!sent);
}

int ping_end = 0;
void *ping_thread(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    int *responded = calloc(MAX_CLIENTS, sizeof(*responded));
    while (!ping_end) {
        sleep(PING_TIME);
        pthread_mutex_lock(&clients_mutex_socket_thread);
        pthread_mutex_lock(&clients_mutex_sending_thread);
        make_log("Server: pinging", 0);
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i] != NULL && responded[i] == 0) {
                char test[1];
                Message message;
                message.length = 0;
                message.type = PING_REQUEST;
                if (send_message_to(clients[i]->socket_fd, &message, test, clients[i]->adr, clients[i]->adr_len) == -1) {
                    make_log("Server: unregistering client due to send message error", 0);
                    unregister_client(clients[i]->client_id);
                }
            }
        }
        usleep(PING_TIMEOUT);
        receive_responses(local_socket, responded);
        receive_responses(inet_socket, responded);
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i] != NULL && responded[i] == 0) {
                unregister_client(clients[i]->client_id);
            }
            responded[i] = 0;
        }
        pthread_mutex_unlock(&clients_mutex_socket_thread);
        pthread_mutex_unlock(&clients_mutex_sending_thread);
    }
    pthread_exit((void *) 0);
}

void receive_responses(int socket, int *responded) {
    struct sockaddr *adr = calloc(1, sizeof(*adr));
    socklen_t adr_len;
    Message message;
    void *data;
    while ((data = receive_message_from(socket, &message, adr, &adr_len)) != NULL) {
        int index;
        if ((index = find_client_index(message.client_id)) != -1 && message.type == PING_RESPONSE) {
            responded[index] = 1;
        }
        free(data);
        memset(adr, '\0', sizeof(*adr));
    }
    free(adr);
}

int find_client_index(int client_id) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] != NULL && clients[i]->client_id == client_id) {
            return i;
        }
    }
    return -1;
}
