//
// Created by adam on 03.05.17.
//

#ifndef SHAREDMEMORYSEM_COMMON_H
#define SHAREDMEMORYSEM_COMMON_H

#define SEMAPHORE_COUNT 10
#define BARBER_FREE_TO_WAKE_UP 0
#define READING_QUEUE 1
#define WRITING_QUEUE 2
#define CHAIR_LOCK 3
#define SHAVING_LOCK 4
#define DONE_LOCK 5
#define BARBER_TURN 6
#define CLIENT_TURN 7
#define BLACKHOLE 8
#define BARBER_READY 9
#define LAST_SEMOP_PID 653655

const int CLIENTS_QUEUE_KEY = SEMAPHORE_COUNT;

typedef struct ClientsQueue {
    int *queue;
    int head;
    int queued;
    int size;
} ClientsQueue;

#endif //SHAREDMEMORYSEM_COMMON_H
