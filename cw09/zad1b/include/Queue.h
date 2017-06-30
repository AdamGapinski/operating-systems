#ifndef THREADSSYNCHRONIZATION_QUEUE_H
#define THREADSSYNCHRONIZATION_QUEUE_H

typedef struct Queue {
    int head;
    int queued;
    int size;
    long *array;
} Queue;

Queue *initQueue(int size);
int queue_empty(Queue *queue);
int queue_full(Queue *queue);
long queue_head_value(Queue *queue);
int enqueue(Queue *queue, long value);
long dequeue(Queue *queue);

#endif //THREADSSYNCHRONIZATION_QUEUE_H
