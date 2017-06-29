#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <memory.h>
#include "include/Utils.h"

void create_threads(int records_num);

void *start_thread(void *records_num) ;

int find_text(char *text);

int read_data(int records_num) ;

void destructor(void *data) ;

int read_records(char **data) ;

typedef struct record_str {
    int id;
    char text[RECORD_SIZE - sizeof(int)];
} record_str;

pthread_t **threads;
pthread_key_t record_buffer_data_key;
int threads_num;
int records_num;
int fd;
char *text_key;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]) {
    threads_num = parseUnsignedIntArg(argc, argv, 1, "number of threads");
    char *filename = parseTextArg(argc, argv, 2, "file name");
    records_num = parseUnsignedIntArg(argc, argv, 3, "number of records");
    text_key = parseTextArg(argc, argv, 4, "text key to find");

    fd = open_file(filename);
    pthread_key_create(&record_buffer_data_key, destructor);
    create_threads(records_num);
    pthread_exit(NULL);
}

void destructor(void *data) {
    char **records = (char **) data;
    for (int i = 0; i < records_num && records[i] != NULL; ++i) {
        free(records[i]);
    }
    free(data);
}

void create_threads(int records_num) {
    threads = calloc((size_t) threads_num, sizeof(*threads));
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    for (int i = 0; i < threads_num; ++i) {
        pthread_t *thread = malloc(sizeof(*thread));
        int *records = calloc(1, sizeof(*records));
        *records = records_num;
        int error_num;
        if ((error_num = pthread_create(thread, &attr, start_thread, records)) != 0) {
            fprintf(stderr, "Error creating thread: %s\n", strerror(error_num));
            exit(EXIT_FAILURE);
        }
        threads[i] = thread;
    }
    pthread_attr_destroy(&attr);
}

void *start_thread(void *records_num) {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    int read_records;
    while ((read_records = read_data(*(int *) records_num)) != 0) {
        char **data = pthread_getspecific(record_buffer_data_key);
        for (int i = 0; i < read_records; ++i) {
            record_str *record = (record_str *) data[i];
            if (find_text(record->text) == 1) {
                printf("Key found in record %d by thread with id %ld\n", record->id, get_thread_id());
            }
        }
    }
    return (void *) 0;
}

int read_data(int records_num) {
    char **data = pthread_getspecific(record_buffer_data_key);
    if (data == NULL) {
        data = calloc((size_t) records_num, sizeof(*data));
        pthread_setspecific(record_buffer_data_key, data);
    }

    pthread_mutex_lock(&file_mutex);
    int records = read_records(data);
    pthread_mutex_unlock(&file_mutex);

    return records;
}

int read_records(char **data) {
    ssize_t read_result;
    int record_num = 0;
    for (record_num = 0; record_num < records_num; ++record_num) {
        data[record_num] = calloc(RECORD_SIZE, sizeof(*data[record_num]));
        if ((read_result = read(fd, data[record_num], RECORD_SIZE * sizeof(*data[record_num]))) == -1) {
            fprintf(stderr, "Error while reading from file in thread %ld\n", get_thread_id());
            pthread_mutex_unlock(&file_mutex);
            pthread_exit((void *) EXIT_FAILURE);
        }
        if (read_result == 0) {
            break;
        }
    }
    return record_num;
}

int find_text(char *text) {
    size_t key_len = strlen(text_key);
    int record_len = RECORD_SIZE - sizeof(int);
    int key_index;
    for (int i = 0; i < record_len; ++i) {
        for (key_index = 0; key_index < key_len && i + key_index < record_len; ++key_index) {
            if (text[i + key_index] != text_key[key_index]) {
                break;
            }
        }
        if (key_index == key_len) {
            return 1;
        }
    }
    return 0;
}
