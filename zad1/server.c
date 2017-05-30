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
#include "include/utils.h"
#include "include/queue.h"

#define UNIX_PATH_MAX 108
#define LISTEN_BACKLOG 50
#define MAX_CLIENTS 22
#define PING_TIME 3
#define PING_TIMEOUT 10000

typedef struct Client {
    char *name;
    int socket_fd;
    int registered;
} Client;

void start_threads();

void *terminal_thread(void *arg) ;

void *socket_thread(void *arg) ;

void *ping_thread(void *arg) ;

void enqueue_operation(int option);

int read_option() ;

int setup_server_inet_socket(int i);

int setup_server_local_socket(int epoll_fd) ;

void *receive_message(int socket_fd, Message *message) ;

void init_clients_array() ;

int try_register_client(int epoll_fd, int server_socket);

int find_slot() ;

int name_available(char *name);

void to_string(Operation *operation, char *buf) ;

Client *find_client(int socket_fd) ;

void *sending_thread(void *arg) ;

void send_operation(Operation *operation) ;

void handle_client_response(int socket_fd);

in_port_t port;
char *path;
Client *clients[MAX_CLIENTS];
const int queue_size = 10;
Queue *operations;
int operation_counter = 0;
pthread_mutex_t operations_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t operations_cond_not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t operations_cond_not_empty = PTHREAD_COND_INITIALIZER;
pthread_mutex_t clients_mutex_sending_thread = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t clients_mutex_socket_thread = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t clients_cond_registered = PTHREAD_COND_INITIALIZER;

int main(int argc, char *argv[]) {
    port = (in_port_t) parseUnsignedIntArg(argc, argv, 1, "TCP port number");
    path = parseTextArg(argc, argv, 2, "Unix local socket path");
    if (strlen(path) >= UNIX_PATH_MAX) {
        fprintf(stderr, "Argument validation error - socket path too long: %s, max %d\n", path, UNIX_PATH_MAX - 1);
        exit(EXIT_FAILURE);
    }
    operations = init_queue(queue_size);
    init_clients_array();
    make_log("\t\t\tserver started", 0);
    srand((unsigned int) time(NULL));
    start_threads();

    pthread_exit((void *) 0);
}

void init_clients_array() {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        clients[i] = NULL;
    }
}

void start_threads() {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_t thread;
    pthread_create(&thread, &attr, terminal_thread, NULL);
    pthread_create(&thread, &attr, socket_thread, NULL);
    pthread_create(&thread, &attr, sending_thread, NULL);
    pthread_create(&thread, &attr, ping_thread, NULL);
    pthread_attr_destroy(&attr);
}

void *terminal_thread(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    make_log("terminal: started", 0);
    while (1) {
        int option = read_option();
        if (option > 0 && option < 5) {
            enqueue_operation(option);
        } else if (option == 5) {
            //todo cancel the other threads
            printf("exited\n");
            break;
        } else {
            printf("unsupported operation\n");
        }
    }
    make_log("terminal: exited", 0);
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

double read_argument(char *info) {
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
    double first_arg, second_arg;
    first_arg = read_argument("operation first argument");
    second_arg = read_argument("operation second argument");
    if (option == DIVISION && second_arg == 0) {
        printf("Division by zero not supported\n");
        return;
    }
    ++operation_counter;
    Operation *operation = init_operation(option, first_arg, second_arg, -1, operation_counter);
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
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    make_log("socket: started", 0);
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("Error epoll_create1");
        exit(EXIT_FAILURE);
    }
    int server_local_socket = setup_server_local_socket(epoll_fd);
    int server_inet_socket = setup_server_inet_socket(epoll_fd);

    struct epoll_event events[MAX_CLIENTS];
    int waited_fds_num;
    while (!socket_thread_end) {
        pthread_mutex_lock(&clients_mutex_socket_thread);
        if ((waited_fds_num = epoll_wait(epoll_fd, events, MAX_CLIENTS + 2, PING_TIME * 1000)) == -1) {
            make_log("Error epoll_wait", 0);
            perror("Error epoll_wait");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < waited_fds_num; ++i) {
            int event_socket_fd = events[i].data.fd;
            if (((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP)) &&
                    (events[i].events & EPOLLIN) == 0) {
                shutdown(event_socket_fd, SHUT_RDWR);
                close(event_socket_fd);
                errno = 0;
            } else if ((event_socket_fd == server_local_socket || event_socket_fd == server_inet_socket) &&
                    (events[i].events & EPOLLIN)) {
                try_register_client(epoll_fd, event_socket_fd);
            } else if (events[i].events & EPOLLIN){
                handle_client_response(event_socket_fd);
            } else {
                make_log("client ready - event %d", events[i].events);
            }
        }
        pthread_mutex_unlock(&clients_mutex_socket_thread);
        usleep(10);
    }
    make_log("socket: exited", 0);
    pthread_exit((void *) 0);
}

void handle_client_response(int socket_fd) {
    Message message;
    void *received_data;
    if ((received_data = receive_message(socket_fd, &message)) != NULL && message.type == OPERATION_RES_MSG) {
        Operation *result = (Operation *) received_data;
        char buf[128];
        to_string(result, buf);

        Client *client;
        if ((client = find_client(result->client_id)) == NULL) {
            printf("result of %d. %s = %lf from unknown\n", result->operation_id, buf,
                   result->result);
        } else {
            printf("result of %d. %s = %lf from %d. %s\n", result->operation_id, buf,
                   result->result, result->client_id, client->name);
        }
        make_log("obtained result %d", (int) result->result);
        make_log("obtained result id %d", result->operation_id);
    }
    if (received_data == NULL) {
        shutdown(socket_fd, SHUT_RDWR);
        close(socket_fd);
    } else {
        free(received_data);
    }
}

int sending_end = 0;
void *sending_thread(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    make_log("sending thread started", 0);
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
        make_log("operation sent %d", operation->operation_id);
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
            if (clients[i] != NULL && clients[i]->registered) {
                registered[registered_iterator] = i;
                ++registered_iterator;
            }
        }
        if (registered_iterator != 0) {
            int rand_registered = rand() % registered_iterator;
            int client_socket_id = clients[registered[rand_registered]]->socket_fd;
            Message message;
            message.length = sizeof(*operation);
            message.type = OPERATION_REQ_MSG;
            operation->client_id = client_socket_id;
            make_log("Server sending operation to client %d", client_socket_id);
            if (send_message(client_socket_id, &message, operation) == 0) {
                sent = 1;
            } else {
                make_log("Error: server sending operation to client", 0);
                sleep(1);
                sent = 0;
            }
        } else {
            make_log("waiting for registered", 0);
            pthread_cond_wait(&clients_cond_registered, &clients_mutex_sending_thread);
            make_log("waited for registered", 0);
            sent = 0;
        }
        pthread_mutex_unlock(&clients_mutex_sending_thread);
        usleep(1000);
    } while (!sent);
}

Client *find_client(int socket_fd) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] != NULL && clients[i]->socket_fd == socket_fd) {
            return clients[i];
        }
    }
    return NULL;
}

int try_register_client(int epoll_fd, int server_socket) {
    int client_socket_fd;
    if ((client_socket_fd = accept(server_socket, NULL, NULL)) == -1) {
        make_log("Error accept", 0);
        return -1;
    }
    int slot_index;
    if ((slot_index = find_slot()) == -1) {
        make_log("No free slot for new client", 0);
        shutdown(client_socket_fd, SHUT_RDWR);
        close(client_socket_fd);
        return -1;
    }
    Message message;
    char *name;
    if ((name = receive_message(client_socket_fd, &message)) == NULL) {
        make_log("Error receiving client name", 0);
        perror("Error receiving client name");
        free(name);
        return -1;
    };
    if (name_available(name) == -1) {
        message.length = 0;
        message.type = NOT_REGISTERED_RES_MSG;
        send_message(client_socket_fd, &message, NULL);
        free(name);
        return -1;
    }
    message.length = 0;
    message.type = REGISTERED_RES_MSG;
    send_message(client_socket_fd, &message, NULL);

    pthread_mutex_lock(&clients_mutex_sending_thread);
    clients[slot_index] = malloc(sizeof(*clients));
    clients[slot_index]->socket_fd = client_socket_fd;
    clients[slot_index]->name = name;
    clients[slot_index]->registered = 1;

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = client_socket_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket_fd, &event) == -1) {
        make_log("Error epoll_ctl add", 0);
        perror("Error epoll_ctl add");
        pthread_mutex_unlock(&clients_mutex_sending_thread);
        return -1;
    }
    pthread_cond_signal(&clients_cond_registered);
    pthread_mutex_unlock(&clients_mutex_sending_thread);
    make_log("accepted connection", 0);
    return 0;
}

int name_available(char *name) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] != NULL && strcmp(clients[i]->name, name) == 0) {
            return -1;
        }
    }
    return 0;
}

int find_slot() {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] == NULL) {
            return i;
        }
    }
    return -1;
}

int setup_server_local_socket(int epoll_fd) {
    int server_local_socket = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (server_local_socket == -1) {
        perror("Error socket");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_un address;
    memset(&address, 0, sizeof(struct sockaddr_un));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, path, sizeof(address.sun_path) - 1);
    if (bind(server_local_socket, (struct sockaddr *) &address, sizeof(address)) == -1) {
        perror("Error bind");
        exit(EXIT_FAILURE);
    }
    if (listen(server_local_socket, LISTEN_BACKLOG) == -1) {
        perror("Error listen");
        exit(EXIT_FAILURE);
    }
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_local_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_local_socket, &event) == -1) {
        perror("Error epoll_ctl add");
        exit(EXIT_FAILURE);
    }
    return server_local_socket;
}

int setup_server_inet_socket(int epoll_fd) {
    int server_inet_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (server_inet_socket == -1) {
        perror("Error socket");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in address;
    memset(&address, 0, sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_port = htobe16(port);
    struct in_addr adr;
    adr.s_addr = INADDR_ANY;
    address.sin_addr = adr;
    if (bind(server_inet_socket, (struct sockaddr *) &address, sizeof(address)) == -1) {
        perror("Error bind");
        exit(EXIT_FAILURE);
    }
    if (listen(server_inet_socket, LISTEN_BACKLOG - 40) == -1) {
        perror("Error listen");
        exit(EXIT_FAILURE);
    }
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_inet_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_inet_socket, &event) == -1) {
        perror("Error epoll_ctl add");
        exit(EXIT_FAILURE);
    }
    return server_inet_socket;
}

int end = 1;
void *ping_thread(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    make_log("ping: started", 0);

    while (end) {
        sleep(PING_TIME);
        pthread_mutex_lock(&clients_mutex_socket_thread);
        pthread_mutex_lock(&clients_mutex_sending_thread);
        make_log("pinging", 0);
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i] != NULL && clients[i]->registered == 1) {
                char test[1];
                if (recv(clients[i]->socket_fd, test, 1, MSG_DONTWAIT | MSG_PEEK) == 1) {
                    break;
                }
                Message message;
                message.length = 0;
                message.type = PING_REQUEST;
                if (send_message(clients[i]->socket_fd, &message, NULL) == -1) {
                    shutdown(clients[i]->socket_fd, SHUT_RDWR);
                    close(clients[i]->socket_fd);
                    free(clients[i]);
                    clients[i] = NULL;
                } else {
                    usleep(PING_TIMEOUT);
                    if (receive_message(clients[i]->socket_fd, &message) == NULL ||
                            message.type != PING_RESPONSE) {
                        shutdown(clients[i]->socket_fd, SHUT_RDWR);
                        close(clients[i]->socket_fd);
                        free(clients[i]);
                        clients[i] = NULL;
                    };
                }
            }
        }
        pthread_mutex_unlock(&clients_mutex_socket_thread);
        pthread_mutex_unlock(&clients_mutex_sending_thread);
        make_log("pinged", 0);
    }
    make_log("ping: exited", 0);
    pthread_exit((void *) 0);
}