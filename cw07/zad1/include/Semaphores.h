#ifndef SHAREDMEMORYSEM_SEMAPHORES_H
#define SHAREDMEMORYSEM_SEMAPHORES_H

#define SEMAPHORE_COUNT 9
#define BARBER_FREE_TO_WAKE_UP 0
#define QUEUE_SYNCHRONIZATION 1
#define CLIENT_LEFT 2
#define CLIENT_PID 3
#define DONE_LOCK 4
#define CHECK_PID 5
#define CHECKING_QUEUE 6
#define CLIENT_READY 7
#define NEXT 8
#define LAST_SEMOP_PID 653655

void removeSemaphores(char *pathname);
void initSemaphores(char *pathname);
void wait_semaphore(int lock_type);
int nowait_semaphore(int lock_type);
void release_semaphore(int lock_type);
void set_semaphore(int lock_type, int val);
int get_semaphore(int lock_type);
int get_lock_info(int lock_type, int info_type);
void wait_semaphore_acquired(int lock_type);

#endif //SHAREDMEMORYSEM_SEMAPHORES_H
