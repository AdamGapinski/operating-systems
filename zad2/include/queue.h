#ifndef THREADSSYNCHRONIZATION_QUEUE_H
#define THREADSSYNCHRONIZATION_QUEUE_H

#define ADDITION 1
#define SUBTRACTION 2
#define MULTIPLICATION 3
#define DIVISION 4

typedef struct Operation {
    double first_argument;
    double second_argument;
    double result;
    int operation_type;
    int client_id;
    int operation_id;
} Operation;

typedef struct Queue {
    int head;
    int queued;
    int size;
    Operation **array;
} Queue;

Queue *init_queue(int size);
Operation *init_operation(int operation, double first_argument, double second_argument,
                          int client_id, int operation_id);
int queue_empty(Queue *queue);
int queue_full(Queue *queue);
int enqueue(Queue *queue, Operation *value);
Operation *dequeue(Queue *queue);

#endif //THREADSSYNCHRONIZATION_QUEUE_H
