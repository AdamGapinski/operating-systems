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

ClientsQueue *initQueue(char *pathname, int proj_id, int size);
void removeQueue(ClientsQueue *queue, char *pathname, int proj_id);
int clients_queue_is_empty(ClientsQueue *queue);
int clients_queue_is_full(ClientsQueue *queue);
int enqueue(ClientsQueue *queue, int value);
int dequeue(ClientsQueue *queue);
void *get_shared_address(int size, char *pathname, int proj_id);
void remove_shared_address(char *pathname, int proj_id, void *adr);

#endif //SHAREDMEMORYSEM_SYNCHRONIZEDQUEUE_H
