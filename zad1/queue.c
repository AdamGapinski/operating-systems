#include <stdlib.h>
#include "include/queue.h"

Queue *init_queue(int size) {
    Queue *queue = malloc(sizeof(*queue));
    queue->head = 0;
    queue->queued = 0;
    queue->size = size;
    queue->array = calloc((size_t) size, sizeof(*queue->array));
    return queue;
}

Operation *init_operation(int operation, int first_argument, int second_argument,
                          int client_id, int operation_id) {
    Operation *result = malloc(sizeof(*result));
    result->operation = operation;
    result->first_argument = first_argument;
    result->second_argument = second_argument;
    result->client_id = client_id;
    result->operation_id = operation_id;
    return result;
}

int queue_empty(Queue *queue) {
    return queue->queued == 0 ? 1 : 0;
}

int queue_full(Queue *queue) {
    return queue->queued == queue->size ? 1 : 0;
}

int enqueue(Queue *queue, Operation *value) {
    if (queue_full(queue)) {
        return -1;
    }
    queue->array[(queue->head + queue->queued) % queue->size] = value;
    queue->queued += 1;
    return 0;
}

Operation *dequeue(Queue *queue) {
    if (queue_empty(queue)) {
        return NULL;
    }
    Operation *result = queue->array[queue->head];
    queue->head = (queue->head + 1) % queue->size;
    queue->queued -= 1;
    return result;
}

