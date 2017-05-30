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

typedef struct Client {
    char *name;
    int socket_fd;
    int active;
} Client;

void start_threads();

void *startTerminalThread(void *arg) ;

void *startSocketThread(void *arg) ;

void *startPingThread(void *arg) ;

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

int try_send_operation(Operation *operation);

in_port_t port;
char *path;
Client *clients[MAX_CLIENTS];
const int queue_size = 10;
Queue *operations;
int operation_counter = 0;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond_not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

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
    pthread_create(&thread, &attr, startTerminalThread, NULL);
    pthread_create(&thread, &attr, startSocketThread, NULL);
    pthread_create(&thread, &attr, startPingThread, NULL);
    pthread_attr_destroy(&attr);
}

void *startTerminalThread(void *arg) {
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
    pthread_mutex_lock(&queue_mutex);
    if (enqueue(operations, operation) == -1) {
        pthread_cond_wait(&queue_cond_not_full, &queue_mutex);
    };
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
    char buf[128];
    to_string(operation, buf);
    printf("Operation %d. scheduled: %s\n", operation->operation_id, buf);
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

void *startSocketThread(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    make_log("socket: started", 0);
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("Error epoll_create1");
        exit(EXIT_FAILURE);
    }
    int server_local_socket = setup_server_local_socket(epoll_fd);
    int server_inet_socket = setup_server_inet_socket(epoll_fd);

    Operation *operation = NULL;
    struct epoll_event events[MAX_CLIENTS];
    int waited_fds_num;

    while (1) {
        pthread_mutex_lock(&queue_mutex);
        if (queue_empty(operations) == 1) {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_nsec += 100000;
            pthread_cond_timedwait(&queue_cond, &queue_mutex, &ts);
        }
        operation = dequeue(operations);
        pthread_cond_signal(&queue_cond_not_full);
        pthread_mutex_unlock(&queue_mutex);
        if (operation != NULL && try_send_operation(operation) == 1) {
            make_log("operation sent", 0);
            operation = NULL;
            free(operation);
        }

        if ((waited_fds_num = epoll_wait(epoll_fd, events, MAX_CLIENTS, 0)) == -1) {
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
                continue;
            } else if (event_socket_fd == server_local_socket && (events[i].events & EPOLLIN)) {
                try_register_client(epoll_fd, server_local_socket);
            } else if (event_socket_fd == server_inet_socket && (events[i].events & EPOLLIN)) {
                try_register_client(epoll_fd, server_inet_socket);
            } else if (events[i].events & EPOLLOUT) {
                Client *client;
                if ((client = find_client(event_socket_fd)) != NULL) {
                    client->active = 1;
                    make_log("client with socked fd: %d ready to write to", events[i].data.fd);
                }
            }
            if (events[i].events & EPOLLIN){
                make_log("client with socked fd: %d ready to read from", event_socket_fd);
                Message message;
                Operation *result;
                if ((result = receive_message(event_socket_fd, &message)) != NULL) {
                    char buf[128];
                    Client *client;
                    to_string(result, buf);
                    if ((client = find_client(result->client_id)) == NULL) {
                        printf("result of %d. %s = %lf from unknown\n", result->operation_id, buf,
                               result->result);
                    } else {
                        printf("result of %d. %s = %lf from %d. %s\n", result->operation_id, buf,
                               result->result, result->client_id, client->name);
                    }
                    make_log("obtained result %d", (int) result->result);
                    make_log("obtained result id %d", result->operation_id);
                    free(result);
                }
            }
            else {
                make_log("client ready - event %d", events[i].events);
            }
        }
    }
    make_log("socket: exited", 0);
    pthread_exit((void *) 0);
}

int try_send_operation(Operation *operation) {
    if (operation == NULL) return -1;
    int active[MAX_CLIENTS];
    int active_iterator = 0;
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] != NULL && clients[i]->active) {
            active[active_iterator] = i;
            ++active_iterator;
        }
    }
    if (active_iterator != 0) {
        int rand_active = rand() % active_iterator;
        int client_socket_id = clients[active[rand_active]]->socket_fd;
        Message message;
        message.length = sizeof(*operation);
        message.type = OPERATION_REQ_MSG;
        operation->client_id = client_socket_id;
        make_log("Server sending operation to client %d", client_socket_id);
        if (send_message(client_socket_id, &message, operation) == -1) {
            make_log("Error: server sending operation to client", 0);
            return -1;
        }
        return 1;
    }
    return -1;
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
    clients[slot_index] = malloc(sizeof(*clients));
    clients[slot_index]->socket_fd = client_socket_fd;
    clients[slot_index]->name = name;
    clients[slot_index]->active = 0;

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLOUT;
    event.data.fd = client_socket_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket_fd, &event) == -1) {
        make_log("Error epoll_ctl add", 0);
        perror("Error epoll_ctl add");
        return -1;
    }
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
    int server_local_socket = socket(AF_UNIX, SOCK_STREAM, 0);
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
    int server_inet_socket = socket(AF_INET, SOCK_STREAM, 0);
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
void *startPingThread(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    make_log("ping: started", 0);
    /*while (end) {
        sleep(PING_TIME);
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i] != NULL && clients[i]->active == 1) {
                Message message;
                send_message(clients[i]->socket_fd, );
            }
        }
    }*/
    make_log("ping: exited", 0);
    pthread_exit((void *) 0);
}