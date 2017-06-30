#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "include/Utils.h"
#include "include/Queue.h"

int *integers_array;

Queue *queue;

const int queue_size = 20;

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_full_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t queue_empty_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t wait_in_queue_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t readers_count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t readers_count_cond = PTHREAD_COND_INITIALIZER;
int readers_count;

int exit_var = 0;

void start_threads(int writers, int readers, long divider);

void *start_writer(void *arg);

void *start_reader(void *arg);

void randomize_array_integers();

void find_divisible(long divider);

void stand_in_queue();

void wait_for_dequeue();

void release_next();

void wait_readers_left();

void increment_readers();

void decrement_readers();

void wait_in_queue();

int main(int argc, char *argv[]) {
    srand((unsigned int) time(NULL));
    int arg_offset = parseVerboseArg(argc, argv);
    int writers = parseUnsignedIntArg(argc, argv, 1 + arg_offset, "number of writers");
    int readers = parseUnsignedIntArg(argc, argv, 2 + arg_offset, "number of readers");
    //when divider argument is not specified, it is defaulted to random value from 1 to 9
    int divider = argc > 3 + arg_offset ?
                  parseUnsignedIntArg(argc, argv, 3 + arg_offset, "reader divider integer") : (rand() % 9) + 1;
    if (divider == 0) {
        fprintf(stderr, "Error: Invalid argument %d. - reader divider integer could not be zero\n", 3 + arg_offset);
        exit(EXIT_FAILURE);
    }
    
    queue = initQueue(queue_size);
    integers_array = init_integer_array();

    start_threads(writers, readers, divider);
    pthread_exit((void *) 0);
}

void start_threads(int writers, int readers, long divider) {
    pthread_t pthread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    //starting writers threads
    for (int i = 0; i < writers; ++i) {
        pthread_create(&pthread, &attr, start_writer, NULL);
    }
    //starting readers threads
    for (int i = 0; i < readers; ++i) {
        pthread_create(&pthread, &attr, start_reader, (void *) divider);
    }
    pthread_attr_destroy(&attr);
    //if any reader or writer was created, release him from the queue
    if (writers + readers > 0) {
        release_next();
    }
}

void *start_writer(void *arg) {
    while (!exit_var) {
        wait_in_queue();    //stand in queue and wait until release_next call
        wait_readers_left();    //wait for readers to end reading
        randomize_array_integers(); //writing
        release_next(); //release next from the queue (reader or writer)
    }
    return (void *) 0;
}

void wait_in_queue() {
    pthread_mutex_lock(&queue_mutex);
    stand_in_queue();   //enqueue
    wait_for_dequeue(); //wait until release_next call
    pthread_mutex_unlock(&queue_mutex);
}

void stand_in_queue() {
    while (enqueue(queue, get_thread_id()) == -1) {
        //if queue is full, then wait beside the queue
        pthread_cond_wait(&queue_full_cond, &queue_mutex);
    }
    //signalise that the queue is no longer empty
    pthread_cond_signal(&queue_empty_cond);
}

void wait_for_dequeue() {
    while(1) {
        //release queue mutex and wait for for release_next call
        pthread_cond_wait(&wait_in_queue_cond, &queue_mutex);
        if (queue_head_value(queue) == get_thread_id()) {
            dequeue(queue);
            //signalise that queue is for sure no longer full
            pthread_cond_signal(&queue_full_cond);
            break;
        } else {
            //if this thread is not first in the queue, then wake another thread
            pthread_cond_signal(&wait_in_queue_cond);
        }
    }
}

void wait_readers_left() {
    pthread_mutex_lock(&readers_count_mutex);
    while (readers_count != 0) {
        //wait until readers_count change
        pthread_cond_wait(&readers_count_cond, &readers_count_mutex);
    }
    pthread_mutex_unlock(&readers_count_mutex);
}

void *start_reader(void *arg) {
    long divider = (long) arg;
    while (!exit_var) {
        wait_in_queue();    //stand in queue and wait until release_next call
        increment_readers();
        /*
         * reader can release other threads from queue even before reading, because readers do not need exclusive
         * access to integers_array. If next in queue is writer, then he will wait until readers_count == 0
         * */
        release_next();
        find_divisible(divider);
        decrement_readers();
    }
    return (void *) 0;
}

void increment_readers() {
    pthread_mutex_lock(&readers_count_mutex);
    ++readers_count;
    pthread_mutex_unlock(&readers_count_mutex);
}

void decrement_readers() {
    pthread_mutex_lock(&readers_count_mutex);
    --readers_count;
    //signalise eventual writer that count of writers has changed and could be zero
    pthread_cond_signal(&readers_count_cond);
    pthread_mutex_unlock(&readers_count_mutex);
}

void release_next() {
    pthread_mutex_lock(&queue_mutex);
    while (queue_empty(queue)) {
        pthread_cond_wait(&queue_empty_cond, &queue_mutex); //wait until queue is no empty
    }
    pthread_cond_signal(&wait_in_queue_cond);   //signalise waiting thread in head of the fifo queue to dequeue
    pthread_mutex_unlock(&queue_mutex);
}

void randomize_array_integers() {
    //randomize from 1 to INT_ARRAY_SIZE values in array
    int to_randomize = (rand() % INT_ARRAY_SIZE) + 1;

    for (int i = 0; i < to_randomize; ++i) {
        int index = rand() % INT_ARRAY_SIZE;
        int value = rand();
        integers_array[index] = value;
        if (verbose) {
            printf("Writer %ld modified variable at [%d] to %d\n", get_thread_id(), index, value);
        }
    }
    printf("Writer %ld modified integers array\n", get_thread_id());
}

void find_divisible(long divider) {
    int count = 0;  //number of divisible integers found
    for (int i = 0; i < INT_ARRAY_SIZE; ++i) {
        if (integers_array[i] % divider == 0) {
            ++count;    //found divisible
            if (verbose) {
                printf("Reader %ld found divisible value %d at index %d\n", get_thread_id(), integers_array[i], i);
            }
        }
    }
    printf("Reader %ld found %d divisible values\n", get_thread_id(), count);
}
