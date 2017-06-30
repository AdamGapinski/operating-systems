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
#define LISTEN_BACKLOG 50
#define MAX_CLIENTS 22
#define OPERATIONS_QUEUE_SIZE 10
#define PING_TIME 4
#define PING_TIMEOUT 10000

typedef struct Client {
    char *name;
    int socket_fd;
    int registered;
} Client;

void start_threads() ;

void *terminal_thread(void *arg) ;

void *socket_thread(void *arg) ;

void *ping_thread(void *arg) ;

void *sending_thread(void *arg) ;

void enqueue_operation(int option) ;

int read_option() ;

int setup_server_inet_socket(int i) ;

int setup_server_local_socket(int epoll_fd) ;

void *receive_message(int socket_fd, Message *message) ;

int try_register_client(int epoll_fd, int server_socket);

int find_slot() ;

int name_available(char *name);

void to_string(Operation *operation, char *buf) ;

Client *find_client(int socket_fd) ;

void send_operation(Operation *operation) ;

void handle_request(int socket_fd);

void cancel_threads(int signo) ;

void release_resources() ;

void handle_operation_result(void *received_data) ;

void unregister_client(int socket_fd) ;

pthread_t terminal_pthread;
pthread_t socket_pthread;
pthread_t sending_pthread;
pthread_t ping_pthread;

in_port_t port;
char *path;
int server_local_socket;
int server_inet_socket;
int binded_local_socket = 0;
int operation_counter = 0;

Queue *operations;
pthread_mutex_t operations_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t operations_cond_not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t operations_cond_not_empty = PTHREAD_COND_INITIALIZER;

Client *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex_sending_thread = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t clients_mutex_socket_thread = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t clients_cond_registered = PTHREAD_COND_INITIALIZER;

int main(int argc, char *argv[]) {
    port = (in_port_t) parse_unsigned_int_arg(argc, argv, 1, "TCP port number");
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
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] != NULL) {
            shutdown(clients[i]->socket_fd, SHUT_RDWR);
            close(clients[i]->socket_fd);
        }
    }
    shutdown(server_local_socket, SHUT_RDWR);
    shutdown(server_inet_socket, SHUT_RDWR);
    close(server_local_socket);
    close(server_inet_socket);
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
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        make_log("Server: error epoll_create", 0);
        perror("Error epoll_create");
        exit(EXIT_FAILURE);
    }
    server_local_socket = setup_server_local_socket(epoll_fd);
    server_inet_socket = setup_server_inet_socket(epoll_fd);
    struct epoll_event events[MAX_CLIENTS];
    int waited_fds_num;
    while (!socket_thread_end) {
        pthread_mutex_lock(&clients_mutex_socket_thread);
        if ((waited_fds_num = epoll_wait(epoll_fd, events, MAX_CLIENTS + 2, PING_TIME * 1000)) == -1) {
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
                shutdown(event_socket_fd, SHUT_RDWR);
                close(event_socket_fd);
            } else if ((event_socket_fd == server_local_socket || event_socket_fd == server_inet_socket) &&
                    (events[i].events & EPOLLIN)) {
                try_register_client(epoll_fd, event_socket_fd);
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

int setup_server_local_socket(int epoll_fd) {
    int socket_fd;
    if ((socket_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1) {
        perror("Error setting socket");
        make_log("Server: error setting unix socket, errno: %d", errno);
        exit(EXIT_FAILURE);
    }
    struct sockaddr_un address;
    memset(&address, 0, sizeof(struct sockaddr_un));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, path, sizeof(address.sun_path) - 1);
    if (bind(socket_fd, (struct sockaddr *) &address, sizeof(address)) == -1) {
        perror("Error setting socket");
        make_log("Server: error binding unix socket, errno: %d", errno);
        exit(EXIT_FAILURE);
    }
    binded_local_socket = 1;
    if (listen(socket_fd, LISTEN_BACKLOG) == -1) {
        perror("Error setting socket");
        make_log("Server: error setting listening unix socket, errno: %d", errno);
        exit(EXIT_FAILURE);
    }
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = socket_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &event) == -1) {
        perror("Error setting socket");
        make_log("Server: error adding unix socket to epoll, errno: %d", errno);
        exit(EXIT_FAILURE);
    }
    return socket_fd;
}

int setup_server_inet_socket(int epoll_fd) {
    int socket_fd;
    if ((socket_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1) {
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
    if (bind(socket_fd, (struct sockaddr *) &address, sizeof(address)) == -1) {
        perror("Error setting socket");
        make_log("Server: error binding inet socket, errno: %d", errno);
        exit(EXIT_FAILURE);
    }
    if (listen(socket_fd, LISTEN_BACKLOG) == -1) {
        perror("Error setting socket");
        make_log("Server: error setting listening inet socket, errno: %d", errno);
        exit(EXIT_FAILURE);
    }
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = socket_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &event) == -1) {
        perror("Error setting socket");
        make_log("Server: error adding inet socket to epoll, errno: %d", errno);
        exit(EXIT_FAILURE);
    }
    return socket_fd;
}

int try_register_client(int epoll_fd, int server_socket) {
    int client_socket_fd;
    int slot_index;
    if ((slot_index = find_slot()) == -1) {
        make_log("Server: no free slot for new client", 0);
        return -1;
    }
    if ((client_socket_fd = accept(server_socket, NULL, NULL)) == -1) {
        make_log("Server: error accepting client connection", 0);
        return -1;
    }
    Message message;
    char *name;
    if ((name = receive_message(client_socket_fd, &message)) == NULL || message.type != NAME_REQ_MSG) {
        if (name != NULL) {
            free(name);
        }
        make_log("Server: error receiving client name", 0);
        shutdown(client_socket_fd, SHUT_RDWR);
        close(client_socket_fd);
        return -1;
    };
    if (name_available(name) == -1) {
        make_log("Server: error client requested name not available", 0);
        message.length = 0;
        message.type = NOT_REGISTERED_RES_MSG;
        send_message(client_socket_fd, &message, NULL);
        shutdown(client_socket_fd, SHUT_RDWR);
        close(client_socket_fd);
        free(name);
        return -1;
    }
    message.length = 0;
    message.type = REGISTERED_RES_MSG;
    if (send_message(client_socket_fd, &message, NULL) == -1) {
        make_log("Server: error sending registered response", 0);
    };

    pthread_mutex_lock(&clients_mutex_sending_thread);
    clients[slot_index] = malloc(sizeof(*clients[slot_index]));
    clients[slot_index]->socket_fd = client_socket_fd;
    clients[slot_index]->name = name;
    clients[slot_index]->registered = 1;

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = client_socket_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket_fd, &event) == -1) {
        make_log("Server: error epoll add client", 0);
        pthread_mutex_unlock(&clients_mutex_sending_thread);
        return -1;
    }
    pthread_cond_signal(&clients_cond_registered);
    pthread_mutex_unlock(&clients_mutex_sending_thread);
    make_log("Server: accepted connection, socket fd: %d", client_socket_fd);
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

void handle_request(int socket_fd) {
    Message message;
    void *received_data = receive_message(socket_fd, &message);
    if (received_data != NULL) {
        switch (message.type) {
            case OPERATION_RES_MSG:
                make_log("Server: received operation result request from %d", socket_fd);
                handle_operation_result(received_data);
                break;
            case UNREGISTER_REQ_MSG:
                make_log("Server: received unregister request from %d", socket_fd);
                unregister_client(socket_fd);
                break;
            default:
                make_log("Server: received unsupported request type %d", message.type);
        }
        free(received_data);
    } else {
        make_log("Server: closing connection %d", socket_fd);
        shutdown(socket_fd, SHUT_RDWR);
        close(socket_fd);
    }
}

void handle_operation_result(void *received_data) {
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
}

Client *find_client(int socket_fd) {
    pthread_mutex_lock(&clients_mutex_sending_thread);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] != NULL && clients[i]->socket_fd == socket_fd) {
            pthread_mutex_unlock(&clients_mutex_sending_thread);
            return clients[i];
        }
    }
    pthread_mutex_unlock(&clients_mutex_sending_thread);
    return NULL;
}

void unregister_client(int socket_fd) {
    pthread_mutex_lock(&clients_mutex_sending_thread);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] != NULL && clients[i]->socket_fd == socket_fd) {
            make_log("Server: shutting down connection and unregistering %d", clients[i]->socket_fd);
            shutdown(socket_fd, SHUT_RDWR);
            close(socket_fd);
            free(clients[i]->name);
            free(clients[i]);
            clients[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex_sending_thread);
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
            if (send_message(client_socket_id, &message, operation) == 0) {
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
    while (!ping_end) {
        sleep(PING_TIME);
        pthread_mutex_lock(&clients_mutex_socket_thread);
        pthread_mutex_lock(&clients_mutex_sending_thread);
        make_log("Server: pinging", 0);
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
                    make_log("Server: error sending ping - closing connection to %d", clients[i]->socket_fd);
                    shutdown(clients[i]->socket_fd, SHUT_RDWR);
                    close(clients[i]->socket_fd);
                    free(clients[i]->name);
                    free(clients[i]);
                    clients[i] = NULL;
                } else {
                    usleep(PING_TIMEOUT);
                    void *ping;
                    if ((ping = receive_message(clients[i]->socket_fd, &message)) == NULL ||
                            message.type != PING_RESPONSE) {
                        make_log("Server error receiving ping: - closing connection to %d", clients[i]->socket_fd);
                        shutdown(clients[i]->socket_fd, SHUT_RDWR);
                        close(clients[i]->socket_fd);
                        free(clients[i]->name);
                        free(clients[i]);
                        clients[i] = NULL;
                    } else {
                        free(ping);
                    };
                }
            }
        }
        pthread_mutex_unlock(&clients_mutex_socket_thread);
        pthread_mutex_unlock(&clients_mutex_sending_thread);
    }
    pthread_exit((void *) 0);
}
