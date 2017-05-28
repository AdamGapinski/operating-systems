#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "include/utils.h"
#include "include/queue.h"

void start_threads();

void *startTerminalThread(void *arg) ;

void *startSocketThread(void *arg) ;

void *startPingThread(void *arg) ;

void schedule_operation(int option);

int choose_client();

int read_option() ;

int port;
char *path;
const int queue_size = 10;
Queue *operations;
int operation_counter = 0;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t queue_cond_full = PTHREAD_COND_INITIALIZER;

int main(int argc, char *argv[]) {
    port = parseUnsignedIntArg(argc, argv, 1, "UDP port number");
    path = parseTextArg(argc, argv, 2, "Unix local socket path");
    operations = init_queue(queue_size);
    make_log("\t\t\tserver started", 0);
    start_threads();
    pthread_exit((void *) 0);
}

void start_threads() {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_t thread;
    pthread_create(&thread, &attr, startTerminalThread, NULL);
    pthread_create(&thread, &attr, startSocketThread, NULL);
    pthread_create(&thread, &attr, startPingThread, NULL);
    pthread_attr_destroy(&attr);
}

void *startTerminalThread(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    make_log("terminal: started", 0);
    while (1) {
        int option = read_option();
        if (option > 0 && option < 5) {
            schedule_operation(option);
            make_log("scheduled operation %d", operation_counter);
        } else if (option == 5) {
            //todo cancel the other threads
            printf("exit");
            break;
        } else {
            printf("unsupported operation");
        }
    }
    make_log("terminal: exited", 0);
    pthread_exit((void *) 0);
}

int read_option() {
    printf("Chose operation:\n");
    printf("1. addition\n");
    printf("2. subtraction\n");
    printf("3. multiplication\n");
    printf("4. division\n");
    printf("5. stop server\n");
    int option = 0;
    scanf("%d", &option);
    return option;
}

void schedule_operation(int option) {
    int first_arg, second_arg;
    printf("operation first argument:\n");
    scanf("%d", &first_arg);
    printf("operation second argument:\n");
    scanf("%d", &second_arg);
    ++operation_counter;
    Operation *operation = init_operation(option, first_arg, second_arg, -1, operation_counter);
    pthread_mutex_lock(&queue_mutex);
    if (enqueue(operations, operation) == -1) {
        pthread_cond_wait(&queue_cond_full, &queue_mutex);
    };
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
}

void *startSocketThread(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    make_log("socket: started", 0);
    while (1) {
        pthread_mutex_lock(&queue_mutex);
        if (queue_empty(operations) == 1) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }
        Operation *operation = dequeue(operations);
        pthread_cond_signal(&queue_cond_full);
        pthread_mutex_unlock(&queue_mutex);
        make_log("processing operation %d", operation->operation_id);
        operation->client_id = choose_client();
        sleep(3);
    }
    make_log("socket: exited", 0);
    pthread_exit((void *) 0);
}

int choose_client() {
    return 0;
}

void *startPingThread(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    make_log("ping: started", 0);

    make_log("ping: exited", 0);
    pthread_exit((void *) 0);
}