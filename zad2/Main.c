#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <memory.h>
#include <signal.h>
#include "include/Utils.h"

void create_threads(int signal);

void *start_thread(void *signal) ;

void join_threads();

int find_text(char *text);

void cancel_threads_async();

int read_data();

void destructor(void *data) ;

int read_records(char **data) ;

void make_test(int signal, char *signal_name, int test);

void set_signal_handler(int signal);

void set_thread_mask(int signal) ;

void processSigArg(int sig);

void routine(void *arg);

typedef struct record_str {
    int id;
    char text[RECORD_SIZE - sizeof(int)];
} record_str;

pthread_t **threads;
pthread_key_t record_buffer_data_key;
const int threads_num = 10;
const int records_num = 2;
int fd;
char *text_key = "aaa";
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]) {
    char *signal_name = parseTextArg(argc, argv, 1, "name of signal");
    int signal = parse_signal_name(signal_name);
    int test = 0;
    if (signal != SIGFPE) {
        test = parseUnsignedIntArg(argc, argv, 2, "test number");
    };
    fd = open_file("/dev/urandom");
    pthread_key_create(&record_buffer_data_key, destructor);
    printf("Main thread PID %d\tTID %ld\n", getpid(), get_thread_id());

    make_test(signal, signal_name, test);
    printf("Main thread: test done, joining threads\n");
    join_threads();
    pthread_mutex_destroy(&file_mutex);
    pthread_key_delete(record_buffer_data_key);
    close(fd);
    return 0;
}

void routine(void *arg) {
    printf("Thread %ld exited.\n", get_thread_id());
}

void signal_handler(int signo) {
    printf("Received %d signal. PID %d\tTID %ld\n", signo, getpid(), get_thread_id());
}

void set_signal_handler(int signal) {
    struct sigaction action;
    action.sa_handler = signal_handler;
    sigfillset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(signal, &action, NULL);
}

void make_test(int signal, char *signal_name, int test) {
    if (test == 4 || signal == SIGFPE) {
        create_threads(signal);
    } else {
        create_threads(-1);
    }
    switch (test) {
        case 0:
            printf("Thread division by zero\n");
            break;
        case 1:
            printf("Sending %s to process\n", signal_name);
            kill(getpid(), signal);
            break;
        case 2:
            printf("Sending %s masked by main thread to process\n", signal_name);
            set_thread_mask(signal);
            kill(getpid(), signal);
            break;
        case 3:
            printf("Sending %s handled by all threads to process\n", signal_name);
            set_signal_handler(signal);
            kill(getpid(), signal);
            break;
        case 4:
            printf("Sending %s masked by thread to thread\n", signal_name);
            pthread_kill(*threads[0], signal);
            break;
        case 5:
            printf("Sending %s handled by thread to thread\n", signal_name);
            set_signal_handler(signal);
            pthread_kill(*threads[0], signal);
            break;
        default:
            fprintf(stderr, "Test number %d. not supported\n", test);
    }
}

void set_thread_mask(int signal) {
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, signal);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);
}

void destructor(void *data) {
    char **records = (char **) data;
    for (int i = 0; i < records_num && records[i] != NULL; ++i) {
        free(records[i]);
    }
    free(data);
}

void create_threads(int signal) {
    threads = calloc((size_t) threads_num, sizeof(*threads));
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    for (int i = 0; i < threads_num; ++i) {
        pthread_t *thread = malloc(sizeof(*thread));
        threads[i] = thread;
        int *signal_pointer = calloc(1, sizeof(*signal_pointer));
        *signal_pointer = signal;
        int error_num;
        if ((error_num = pthread_create(thread, &attr, start_thread, signal_pointer)) != 0) {
            fprintf(stderr, "Error creating thread: %s\n", strerror(error_num));
            exit(EXIT_FAILURE);
        }
    }
    pthread_attr_destroy(&attr);
}

void join_threads() {
    for (int i = 0; i < threads_num; ++i) {
        void *ret;
        int error_num;
        if ((error_num = pthread_join(*threads[i], &ret)) != 0) {
            fprintf(stderr, "Error joining thread: %s\n", strerror(error_num));
            exit(EXIT_FAILURE);
        }
    }
}

void *start_thread(void *signal) {
    int sig = *(int *) signal;
    //checking if it is the last thread to start
    if (pthread_equal(pthread_self(), *threads[0])) {
        processSigArg(sig);
    }
    pthread_cleanup_push(routine, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    int read_records;
    while ((read_records = read_data()) != 0) {
        char **data = pthread_getspecific(record_buffer_data_key);
        for (int i = 0; i < read_records; ++i) {
            record_str *record = (record_str *) data[i];
            if (find_text(record->text) == 1) {
                pthread_mutex_lock(&file_mutex);
                printf("Key found by thread with id %ld\n", get_thread_id());
                cancel_threads_async();
                pthread_mutex_unlock(&file_mutex);
                pthread_exit((void *) 0);
            }
        }
    }
    pthread_cleanup_pop(1);
    return (void *) 0;
}

void processSigArg(int sig) {
    if (sig == SIGFPE) {
        // Assigning to sig to avoid unused variable warning
        sig = 10 / 0;
        printf("Thread %ld divided by zero\n", get_thread_id());
    } else if (sig != -1) {
        set_thread_mask(sig);
    }
}

int read_data() {
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

void cancel_threads_async() {
    for (int i = 0; i < threads_num; ++i) {
        if (threads[i] != NULL && pthread_equal(*threads[i], pthread_self()) == 0) {
            pthread_cancel(*threads[i]);
        }
    }
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
