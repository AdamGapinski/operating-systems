#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include "include/Utils.h"

int *integers_array;
sem_t write_semaphore;
sem_t writers_count;
sem_t readers_count;
sem_t checking_writers_count;
sem_t checking_readers_count;

int exit_var = 0;

void start_threads(int writers, int readers, long divider);

void *start_writer(void *arg);

void *start_reader(void *arg);

void init_semaphores();

void randomize_array_integers();

void find_divisible(long divider);

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

    init_semaphores();
    integers_array = init_integer_array();

    start_threads(writers, readers, divider);
    pthread_exit((void *) 0);
}

void init_semaphores() {
    sem_init(&writers_count, 0, 0);
    sem_init(&readers_count, 0, 0);
    sem_init(&write_semaphore, 0, 1);   //unlocked
    sem_init(&checking_writers_count, 0, 1);    //unlocked
    sem_init(&checking_readers_count, 0, 1);    //unlocked
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
}

void *start_writer(void *arg) {
    while (!exit_var) {
        //setting number of writers is critical section to allow the reader to get number of writers
        //and eventually lock the write_semaphore atomicly
        sem_wait(&checking_writers_count);
        sem_post(&writers_count);   //increment number of writers
        sem_post(&checking_writers_count);

        //writing access is exclusive between writers
        sem_wait(&write_semaphore);
        //writing
        randomize_array_integers();
        sem_post(&write_semaphore);

        sem_wait(&writers_count);   //decrement number of writers
    }
    return (void *) 0;
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

void *start_reader(void *arg) {
    long divider = (long) arg;
    int writers;
    int readers;
    while (!exit_var) {
        //checking number of writers and eventually locking the write_semaphore atomicly when no writers
        sem_wait(&checking_readers_count);  //to block next readers threads when checking if no readers left
        sem_wait(&checking_writers_count);
        sem_getvalue(&writers_count, &writers);
        if (writers == 0) {
            sem_trywait(&write_semaphore);  //reader threads does not have to have exclusive write access
            errno = 0;  //sem_trywait could set that to EAGAIN when many threads are reading
            sem_post(&checking_writers_count);
            sem_post(&checking_readers_count);

            sem_post(&readers_count);   //incrementing number of readers
            find_divisible(divider);
            sem_wait(&readers_count);   //decrementing number of readers

            //checking number of readers is critical section to avoid entering next reader right after check
            sem_wait(&checking_readers_count);
            sem_getvalue(&readers_count, &readers);
            if (readers == 0) {
                //no readers left, let the writers write
                sem_post(&write_semaphore);
            }
            sem_post(&checking_readers_count);
        } else {
            //there were writers waiting to write or writing
            sem_post(&checking_writers_count);
            sem_post(&checking_readers_count);
        }
    }
    return (void *) 0;
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
