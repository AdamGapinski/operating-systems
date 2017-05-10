//
// Created by adam on 03.05.17.
//

#ifndef SHAREDMEMORYSEM_COMMON_H
#define SHAREDMEMORYSEM_COMMON_H

const int CLIENTS_QUEUE_KEY = 240;
int pid = 0;

typedef struct ClientsQueue {
    int *queue;
    int head;
    int queued;
    int size;
} ClientsQueue;

#endif //SHAREDMEMORYSEM_COMMON_H
