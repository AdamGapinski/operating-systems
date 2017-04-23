//
// Created by adam on 22.04.17.
//

#ifndef IPCCOMMUNICATION_COMMON_H
#define IPCCOMMUNICATION_COMMON_H

const char PROJ_ID = 'm';

#define MSG_MAX_SIZE 512

typedef enum {
    REGISTER, ECHO, CAPS, TIME, EXIT
} Request;

typedef struct message {
    long message_type;
    char message[MSG_MAX_SIZE];
    pid_t client;
} message;

#endif //IPCCOMMUNICATION_COMMON_H
