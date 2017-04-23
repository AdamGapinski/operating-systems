//
// Created by adam on 22.04.17.
//

#ifndef IPCCOMMUNICATION_COMMON_H
#define IPCCOMMUNICATION_COMMON_H

const char *SERVER_QUEUE_NAME = "/server_queue_name";

#define MAX_ON_QUEUE 10
#define MAX_MSG_SIZE sizeof(message)
#define MAX_MSG_TEXT_SIZE 8000
#define MAX_CLIENTS 20

typedef enum {
    UNDEFINED, REGISTER, ECHO, CAPS, TIME, EXIT
} Request;

typedef struct message {
    char message_type;
    char message[MAX_MSG_TEXT_SIZE];
    pid_t client;
} message;

#endif //IPCCOMMUNICATION_COMMON_H
