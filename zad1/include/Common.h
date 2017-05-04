//
// Created by adam on 03.05.17.
//

#ifndef SHAREDMEMORYSEM_COMMON_H
#define SHAREDMEMORYSEM_COMMON_H

const int CLIENTS_QUEUE_KEY = 23;
struct ClientsQueue {
    int *queue;
    int head;
    int queued;
    int size;
} typedef ClientsQueue;
#endif //SHAREDMEMORYSEM_COMMON_H
