//
// Created by adam on 10.05.17.
//

#ifndef SHAREDMEMORYSEM_SYNCHRONIZEDQUEUE_H
#define SHAREDMEMORYSEM_SYNCHRONIZEDQUEUE_H

typedef struct ClientsQueue {
    int *head;
    int *queued;
    int *size;
    int *array;
} ClientsQueue;

ClientsQueue *initQueue(char *global_name, int size);
void removeQueue(ClientsQueue *queue, char *global_name);
int clients_queue_is_empty(ClientsQueue *queue);
int clients_queue_is_full(ClientsQueue *queue);
int enqueue(ClientsQueue *queue, int value);
int dequeue(ClientsQueue *queue);
void *get_shared_address(int size, char *global_name);
void remove_shared_address(char *global_name, void *adr, int size);

#endif //SHAREDMEMORYSEM_SYNCHRONIZEDQUEUE_H
