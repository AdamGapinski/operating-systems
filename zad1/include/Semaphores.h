#ifndef SHAREDMEMORYSEM_SEMAPHORES_H
#define SHAREDMEMORYSEM_SEMAPHORES_H

#define SEMAPHORE_COUNT 10
#define BARBER_FREE_TO_WAKE_UP 0
#define QUEUE_SYNCHRONIZATION 1
#define CHAIR_LOCK 3
#define SHAVING_LOCK 4
#define DONE_LOCK 5
#define BARBER_TURN 6
#define CLIENT_TURN 7
#define BLACKHOLE 8
#define BARBER_READY 9
#define LAST_SEMOP_PID 653655

void removeSemaphores(char *pathname);
void initSemaphores(char *pathname);
void wait_semaphore(int lock_type);
void release_semaphore(int lock_type);
void set_semaphore(int lock_type, int val);
int get_semaphore(int lock_type);
int get_lock_info(int lock_type, int info_type);
void wait_semaphore_acquired(int lock_type);

#endif //SHAREDMEMORYSEM_SEMAPHORES_H
