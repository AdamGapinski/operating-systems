#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <endian.h>
#include "include/utils.h"

int validate_domain(char *domain);

void wait_for_local_requests(char *socket_path);

void wait_for_inet_requests(char *address, int port);

void request_name_local(char *name, char *string);

void request_name_inet(char *name, char *address, int port);

int connect_to_server(char *socket_path) ;

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
        request_name_local(client_name, socket_path);
        wait_for_local_requests(socket_path);
    } else if (domain_option == 1){
        char *address = parseTextArg(argc, argv, 3, "IPv4 address");
        int port = parseUnsignedIntArg(argc, argv, 4, "port number");
        request_name_inet(client_name, address, port);
        wait_for_inet_requests(address, port);
    }
    return 0;
}

void request_name_local(char *name, char *socket_path) {
    int server_socket_fd = connect_to_server(socket_path);

    Message *message;
    size_t msg_bytes = sizeof(message->type) + sizeof(message->length) + strlen(name) + 1;
    void *msg_data_pointer = calloc(msg_bytes, sizeof(char));
    message = (Message *) msg_data_pointer;
    message->type = NAME_REQ_MSG;
    message->length = (short) (strlen(name) + 1);
    char *data_pointer = ((char *) message) + sizeof(*message);
    strncpy(data_pointer, name, strlen(name) + 1);

    make_log("client: type %d", message->type);
    make_log("client: length %d", message->length);
    make_log("client: message length %d", (int) msg_bytes);
    make_log(data_pointer, 0);

    message->type = htobe16(message->type);
    message->length = htobe16(message->length);
    //make_log("client: length after htobe16 %d", message->length);
    ssize_t send_result;
    if ((send_result = send(server_socket_fd, msg_data_pointer, msg_bytes, 0)) != msg_bytes) {
        make_log("client: sending error", 0);
        exit(EXIT_FAILURE);
    }
    if (shutdown(server_socket_fd, SHUT_RDWR) == -1) {
        perror("shutdown error");
        exit(EXIT_FAILURE);
    };

    free(msg_data_pointer);
}

int connect_to_server(char *socket_path) {
    int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un address;
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, socket_path);
    if (connect(socket_fd, (struct sockaddr *) &address, sizeof(address)) == -1) {
        perror("Connection error");
        exit(EXIT_FAILURE);
    }
    return socket_fd;
}

void request_name_inet(char *name, char *address, int port) {

}

void wait_for_local_requests(char *socket_path) {
    /*int socketd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un address;
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, socket_path);

    if (connect(socketd, (struct sockaddr *) &address, sizeof(address)) == -1) {
        perror("Connection error");
        exit(EXIT_FAILURE);
    }

    while (1) {
        ssize_t send_result;
        size_t buf_len = 100;
        char buf[buf_len];
        strncpy(buf, "some important message", buf_len);

        if ((send_result = send(socketd, buf, buf_len, 0)) == -1) {
            handle_error("send error");
        } else {
            printf("sent %d bytes\n", (int) send_result);
        }

        sleep(1);
    }

    if (shutdown(socketd, SHUT_RDWR) == -1) {
        printf("errno %d ", errno);
        perror("shutdown error");
        exit(EXIT_FAILURE);
    }

    if (close(socketd) == -1) {
        printf("errno %d ", errno);
        perror("close error");
        exit(EXIT_FAILURE);
    }*/
}

void wait_for_inet_requests(char *address, int port) {
    while(1) {

    }
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