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

void wait_for_requests(int server_socket_id);

int solve_operation(Operation *operation, Operation *result) ;

int connect_inet(char *address, int port) ;

int main(int argc, char *argv[]) {
    char *client_name = parseTextArg(argc, argv, 1, "client name");
    if (strlen(client_name) >= CLIENT_MAX_NAME) {
        fprintf(stderr, "Argument validation error - name too long: %s\n", client_name);
        exit(EXIT_FAILURE);
    }
    char *domain = parseTextArg(argc, argv, 2, "domain: unix or inet");
    int domain_option = validate_domain(domain);
    int server_socket_fd = 0;
    if (domain_option == 0) {
        char *socket_path = parseTextArg(argc, argv, 3, "unix socket path");
        server_socket_fd = connect_local(socket_path);
    } else if (domain_option == 1){
        char *address = parseTextArg(argc, argv, 3, "IPv4 address");
        int port = parseUnsignedIntArg(argc, argv, 4, "port number");
        server_socket_fd = connect_inet(address, port);
    }

    Message message;
    message.type = NAME_REQ_MSG;
    message.length = (short) (strlen(client_name) + 1);
    if (send_message(server_socket_fd, &message, client_name) == -1) exit(EXIT_FAILURE);
    char *response;
    if ((response = receive_message(server_socket_fd, &message)) == NULL) exit(EXIT_FAILURE);
    if (strcmp(response, "registered") == 0) {
        free(response);
        wait_for_requests(server_socket_fd);
    } else if (strcmp(response, "not registered") == 0) {
        make_log("Error: client could not get registered", 0);
        fprintf(stderr, "Error: client could not get registered\n");
        exit(EXIT_FAILURE);
    } else {
        make_log("Error: Client does not recognize server response", 0);
        fprintf(stderr, "Error: Client does not recognize server response\n");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);

    return 0;
}

void wait_for_requests(int server_socket_id) {
    Message message;
    Operation *operation;
    Operation result;
    while ((operation = (Operation *) (receive_message(server_socket_id, &message)))) {
        if (solve_operation(operation, &result) == 0) {
            result.operation_id = operation->operation_id;
            result.first_argument = operation->first_argument;
            result.second_argument = operation->second_argument;
            result.client_id = operation->client_id;
            result.operation = operation->operation;
            message.length = sizeof(result);
            message.type = OPERATION_RES_MSG;
            if (send_message(server_socket_id, &message, &result) == -1) {
                make_log("Error: client could not sent back result of operation %d", result.operation_id);
                fprintf(stderr, "Error: client could not sent back result of operation %d\n", result.operation_id);
            };
            free(operation);
        }
    }
}

int solve_operation(Operation *operation, Operation *result) {
    switch (operation->operation) {
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
            make_log("Error: invalid operation number %d", operation->operation);
            fprintf(stderr, "Error: invalid operation number %d\n", operation->operation);
            return -1;
    }
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
