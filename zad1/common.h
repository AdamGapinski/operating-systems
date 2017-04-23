//
// Created by adam on 22.04.17.
//

#ifndef IPCCOMMUNICATION_COMMON_H
#define IPCCOMMUNICATION_COMMON_H

const char PROJ_ID = 'm';
#define MAX_CLIENTS 20
#define KEY_SEEDS_SIZE 25
const char KEY_SEEDS[KEY_SEEDS_SIZE] = {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
                                        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'm',
                                        'n', 'Q', 'W', 'E', 'R'};
typedef enum {
    UNDEFINED, REGISTER, ECHO, CAPS, TIME, EXIT
} Request;

#define MSG_MAX_SIZE 1024
typedef struct message {
    long message_type;
    char message[MSG_MAX_SIZE];
    pid_t client;
} message;

#endif //IPCCOMMUNICATION_COMMON_H
