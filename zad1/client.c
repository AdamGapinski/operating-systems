#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <endian.h>
#include <float.h>
#include "include/utils.h"
#include "include/queue.h"
#define _BSD_SOURCE

int validate_domain(char *domain);

int connect_local(char *socket_path) ;

void wait_for_requests();

int get_result(Operation *operation, Operation *result) ;

int connect_inet(char *address, int port) ;

void handle_operation_request(void *received_data);

void handle_ping_request();

int try_register(char *client_name) ;

int server_socket_fd = 0;

int main(int argc, char *argv[]) {
    char *client_name = parseTextArg(argc, argv, 1, "client name");
    if (strlen(client_name) >= CLIENT_MAX_NAME) {
        fprintf(stderr, "Argument validation error - name too long: %s\n", client_name);
        exit(EXIT_FAILURE);
    }
    char *domain = parseTextArg(argc, argv, 2, "domain: unix or inet");
    int domain_option = validate_domain(domain);
    if (domain_option == 0) {
        char *socket_path = parseTextArg(argc, argv, 3, "unix socket path");
        server_socket_fd = connect_local(socket_path);
    } else if (domain_option == 1){
        char *address = parseTextArg(argc, argv, 3, "IPv4 address");
        int port = parseUnsignedIntArg(argc, argv, 4, "port number");
        server_socket_fd = connect_inet(address, port);
    }

    if (try_register(client_name) == 0) {
        printf("Client registered with name %s\n", client_name);
        wait_for_requests();
    } else {
        fprintf(stderr, "Error: client could not get registered\n");
        exit(EXIT_FAILURE);
    };
    exit(EXIT_SUCCESS);
}

int try_register(char *client_name) {
    Message message;
    message.type = NAME_REQ_MSG;
    message.length = (short) (strlen(client_name) + 1);
    if (send_message(server_socket_fd, &message, client_name) == -1) {
        fprintf(stderr, "Error: sending registration request message\n");
        return -1;
    };

    void *received_data;
    if ((received_data = receive_message(server_socket_fd, &message)) == NULL) {
        fprintf(stderr, "Error: receiving registration response message\n");
        return -1;
    };
    free(received_data);

    switch (message.type) {
        case REGISTERED_RES_MSG:
            return 0;
        case NOT_REGISTERED_RES_MSG:
            return -1;
        default:
            fprintf(stderr, "Error: client received unexpected message type\n");
            return -1;
    }
}

void wait_for_requests() {
    Message message;
    void *received_data;
    while ((received_data = receive_message(server_socket_fd, &message))) {
        switch (message.type) {
            case OPERATION_REQ_MSG:
                handle_operation_request(received_data);
                free(received_data);
                break;
            case PING_REQUEST:
                handle_ping_request();
                free(received_data);
                break;
            default:
                make_log("Client received unsupported message type", 0);
        }
    }
}

void handle_ping_request() {
    Message message;
    message.length = 0;
    message.type = PING_RESPONSE;
    if (send_message(server_socket_fd, &message, NULL) == -1) {
        make_log("Error: client could not sent ping response", 0);
        fprintf(stderr, "Error: client could not sent ping response\n");
    } else {
        make_log("Client sent message type %d", PING_RESPONSE);
    };
}

void handle_operation_request(void *received_data) {
    Operation *requested_operation = (Operation *) received_data;
    Operation result;
    if (get_result(requested_operation, &result) == 0) {
        Message message;
        message.length = sizeof(result);
        message.type = OPERATION_RES_MSG;
        if (send_message(server_socket_fd, &message, &result) == -1) {
            make_log("Error: client could not sent result of operation %d", result.operation_id);
            fprintf(stderr, "Error: client could not sent result of operation %d\n", result.operation_id);
        } else {
            make_log("Client sent message %d", requested_operation->operation_id);
        };
    }
}

int get_result(Operation *operation, Operation *result) {
    switch (operation->operation_type) {
        case ADDITION:
            result->result = operation->first_argument + operation->second_argument;
            break;
        case SUBTRACTION:
            result->result = operation->first_argument - operation->second_argument;
            break;
        case MULTIPLICATION:
            result->result = operation->first_argument * operation->second_argument;
            break;
        case DIVISION:
            if (operation->second_argument == 0) {
                make_log("Error: invalid operation - dividing by zero", 0);
                fprintf(stderr, "Error: invalid operation - dividing by zero\n");
                return -1;
            } else {
                result->result = operation->first_argument / operation->second_argument;
            }
            break;
        default:
            make_log("Error: invalid operation number %d", operation->operation_type);
            fprintf(stderr, "Error: invalid operation number %d\n", operation->operation_type);
            return -1;
    }
    result->operation_id = operation->operation_id;
    result->first_argument = operation->first_argument;
    result->second_argument = operation->second_argument;
    result->client_id = operation->client_id;
    result->operation_type = operation->operation_type;
    return 0;
}

int connect_local(char *socket_path) {
    int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un address;
    memset(&address, 0, sizeof(struct sockaddr_un));
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, socket_path);
    if (connect(socket_fd, (struct sockaddr *) &address, sizeof(address)) == -1) {
        perror("Connection error");
        exit(EXIT_FAILURE);
    }
    return socket_fd;
}

int connect_inet(char *ipv4_address, int port) {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    memset(&address, 0, sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_port = htobe16((in_port_t) port);
    struct in_addr iadr;
    if (inet_aton(ipv4_address, &iadr) == 0) {
        fprintf(stderr, "Error: invalid address\n");
        exit(EXIT_FAILURE);
    }
    address.sin_addr = iadr;
    if (connect(socket_fd, (struct sockaddr *) &address, sizeof(address)) == -1) {
        perror("Connection error");
        exit(EXIT_FAILURE);
    }
    return socket_fd;
}

int validate_domain(char *domain) {
    for (int i = 0; domain[i] != '\0'; ++i) {
        domain[i] = (char) tolower(domain[i]);
    }
    if (strcmp(domain, "unix")  == 0) {
        return 0;
    } else if (strcmp(domain, "inet") == 0) {
        return 1;
    } else {
        fprintf(stderr, "Argument validation error - unrecognized domain: %s\n", domain);
        exit(EXIT_FAILURE);
    }
}
