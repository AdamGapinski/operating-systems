//
// Created by adam on 10.05.17.
//

#ifndef SHAREDMEMORYSEM_SYNCHRONIZEDQUEUE_H
#define SHAREDMEMORYSEM_SYNCHRONIZEDQUEUE_H

int clients_queue_is_empty(int *queued);
int clients_queue_is_full(int *queued, int *size);
int enqueue(int *queue, int *head, int *queued, int size, int value);
int dequeue(int *queue, int *head, int *queued, int size);

#endif //SHAREDMEMORYSEM_SYNCHRONIZEDQUEUE_H
