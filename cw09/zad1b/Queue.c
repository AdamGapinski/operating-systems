#include <stdlib.h>
#include "include/Queue.h"

Queue *initQueue(int size) {
    Queue *queue = malloc(sizeof(*queue));
    queue->head = 0;
    queue->queued = 0;
    queue->size = size;
    queue->array = calloc((size_t) size, sizeof(*queue->array));
    return queue;
}

int queue_empty(Queue *queue) {
    return queue->queued == 0 ? 1 : 0;
}

int queue_full(Queue *queue) {
    return queue->queued == queue->size ? 1 : 0;
}

long queue_head_value(Queue *queue) {
    if (queue_empty(queue)) {
        return -1;
    }
    return queue->array[queue->head];
}

int enqueue(Queue *queue, long value) {
    if (queue_full(queue)) {
        return -1;
    }
    queue->array[(queue->head + queue->queued) % queue->size] = value;
    queue->queued += 1;
    return 0;
}

long dequeue(Queue *queue) {
    if (queue_empty(queue)) {
        return -1;
    }
    long result = queue->array[queue->head];
    queue->head = (queue->head + 1) % queue->size;
    queue->queued -= 1;
    return result;
}

