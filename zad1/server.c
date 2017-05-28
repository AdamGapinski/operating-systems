#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include "include/utils.h"
#include "include/queue.h"
#define UNIX_PATH_MAX 108
#define LISTEN_BACKLOG 50
#define MAX_CLIENTS 22

void start_threads();

void *startTerminalThread(void *arg) ;

void *startSocketThread(void *arg) ;

void *startPingThread(void *arg) ;

void schedule_operation(int option);

int choose_client();

int read_option() ;

int setup_server_inet_socket(int i);

int setup_server_local_socket(int epoll_fd) ;

in_port_t port;
char *path;
const int queue_size = 10;
Queue *operations;
int operation_counter = 0;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond_not_full = PTHREAD_COND_INITIALIZER;

int main(int argc, char *argv[]) {
    port = (in_port_t) parseUnsignedIntArg(argc, argv, 1, "UDP port number");
    path = parseTextArg(argc, argv, 2, "Unix local socket path");
    if (strlen(path) >= UNIX_PATH_MAX) {
        fprintf(stderr, "Argument validation error - socket path too long: %s, max %d\n", path, UNIX_PATH_MAX - 1);
        exit(EXIT_FAILURE);
    }
    operations = init_queue(queue_size);
    make_log("\t\t\tserver started", 0);
    start_threads();

    pthread_exit((void *) 0);
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
            schedule_operation(option);
            make_log("scheduled operation %d", operation_counter);
        } else if (option == 5) {
            //todo cancel the other threads
            printf("exit");
            break;
        } else {
            printf("unsupported operation");
        }
    }
    make_log("terminal: exited", 0);
    pthread_exit((void *) 0);
}

int read_option() {
    printf("Chose operation:\n");
    printf("1. addition\n");
    printf("2. subtraction\n");
    printf("3. multiplication\n");
    printf("4. division\n");
    printf("5. stop server\n");
    int option = 0;
    scanf("%d", &option);
    return option;
}

void schedule_operation(int option) {
    int first_arg, second_arg;
    printf("operation first argument:\n");
    scanf("%d", &first_arg);
    printf("operation second argument:\n");
    scanf("%d", &second_arg);
    ++operation_counter;
    Operation *operation = init_operation(option, first_arg, second_arg, -1, operation_counter);
    pthread_mutex_lock(&queue_mutex);
    if (enqueue(operations, operation) == -1) {
        pthread_cond_wait(&queue_cond_not_full, &queue_mutex);
    };
    pthread_mutex_unlock(&queue_mutex);
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
    while (1) {
        struct epoll_event events[MAX_CLIENTS];
        int fd_num;
        make_log("socket epoll_wait", 0);
        if ((fd_num = epoll_wait(epoll_fd, events, MAX_CLIENTS, 100000)) == -1) {
            perror("Error epoll_wait");
            exit(EXIT_FAILURE);
        } else {
            make_log("socket epoll_waited %d", fd_num);
            for (int i = 0; i < fd_num; ++i) {
                if (events[i].data.fd == server_local_socket) {
                    int cl_sock_fd;
                    struct sockaddr_un client_address;
                    socklen_t adr_size = sizeof(struct sockaddr_un);
                    if ((cl_sock_fd = accept(server_local_socket, (struct sockaddr *) &client_address, &adr_size)) == -1) {
                        perror("Error accept local");
                    } else {
                        struct epoll_event event;
                        event.events = EPOLLIN;
                        event.data.fd = cl_sock_fd;
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cl_sock_fd, &event) == -1) {
                            perror("Error epoll_ctl add");
                        }
                        make_log("accepted connection", 0);
                    };
                } else if (events[i].data.fd == server_inet_socket) {
                    int cl_sock_fd;
                    struct sockaddr_in client_address;
                    socklen_t adr_size = sizeof(struct sockaddr_in);
                    if ((cl_sock_fd = accept(server_inet_socket, (struct sockaddr *) &client_address, &adr_size)) == -1) {
                        perror("Error accept inet");
                    } else {
                        struct epoll_event event;
                        event.events = EPOLLIN;
                        event.data.fd = cl_sock_fd;
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cl_sock_fd, &event) == -1) {
                            perror("Error epoll_ctl add");
                        }
                        make_log("accepted connection", 0);
                    };
                } else {
                    int received;
                    int buf_len = CLIENT_MAX_NAME;
                    char buf[buf_len];
                    received = (int) recv(events[i].data.fd, buf, (size_t) buf_len, 0);
                    if (received == 0) {
                        make_log("client closed connection\n", 0);
                    } else if (received == -1) {
                        perror("Error recv");
                    } else {
                        make_log("received %d bytes", received);
                        make_log(buf, 0);
                    }
                }
            }
        }
        pthread_mutex_lock(&queue_mutex);
        if (queue_empty(operations) == 0) {
            Operation *operation = dequeue(operations);
            pthread_cond_signal(&queue_cond_not_full);
            pthread_mutex_unlock(&queue_mutex);
            make_log("processing operation %d", operation->operation_id);
            operation->client_id = choose_client();
        } else {
            pthread_mutex_unlock(&queue_mutex);
        }
    }
    make_log("socket: exited", 0);
    pthread_exit((void *) 0);
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

/*void accept_client() {
    int clientd;
    struct sockaddr_un client_address;
    socklen_t client_address_struct_size = sizeof(struct sockaddr_un);
    if ((clientd = accept(server_local_socket, (struct sockaddr *) &client_address, &client_address_struct_size)) == -1) {
        perror("Error accept");
        exit(EXIT_FAILURE);
    }

    while (1) {
        int rec_result;
        int buf_len = CLIENT_MAX_NAME;
        char buf[buf_len];
        rec_result = (int) recv(clientd, buf, (size_t) buf_len, 0);

        if (rec_result == 0) {
            printf("client closed connection\n");
        } else if (rec_result == -1) {
            perror("Error receive");
            exit(EXIT_FAILURE);
        } else {
            printf("received %d bytes\n", rec_result);
            printf("received %s\n", buf);
        }
        sleep(1);
    }

    if (shutdown(server_local_socket, SHUT_RDWR) == -1) {
        printf("errno %d ", errno);
        perror("shutdown error");
        exit(EXIT_FAILURE);
    }

    if (close(server_local_socket) == -1) {
        printf("errno %d ", errno);
        perror("close error");
        exit(EXIT_FAILURE);
    }

    unlink(path);
}*/

int choose_client() {
    return 0;
}

void *startPingThread(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    make_log("ping: started", 0);

    make_log("ping: exited", 0);
    pthread_exit((void *) 0);
}