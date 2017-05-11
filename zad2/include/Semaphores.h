#ifndef SHAREDMEMORYSEM_SEMAPHORES_H
#define SHAREDMEMORYSEM_SEMAPHORES_H

#define SEMAPHORE_COUNT 7
#define BARBER_FREE_TO_WAKE_UP 0
#define QUEUE_SYNCHRONIZATION 1
#define CLIENT_LEFT 2
#define DONE_LOCK 3
#define CHECKING_QUEUE 4
#define CLIENT_READY 5
#define NEXT 6
#define LAST_SEMOP_PID 653655

void removeSemaphores(char *pathname);
void initSemaphores(char *pathname);
void wait_semaphore(int lock_type);
void timed_wait_semaphore(int lock_type, int nsec);
int nowait_semaphore(int lock_type);
void release_semaphore(int lock_type);

#endif //SHAREDMEMORYSEM_SEMAPHORES_H
