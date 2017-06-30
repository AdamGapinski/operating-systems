#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../include/Semaphores.h"
#include "../include/Common.h"

void at_exit() {
    removeSemaphores("./");
}

void simpleTest() {
    if (fork() == 0) {
        printf("forked pid %d\n", getpid());
        sleep(3);
        printf("BARBER_FREE_TO_WAKE_UP released\n");
        release_semaphore(BARBER_FREE_TO_WAKE_UP);
        sleep(1);
        if (nowait_semaphore(BARBER_FREE_TO_WAKE_UP) == 1) {
            printf("failed: BARBER_FREE_TO_WAKE_UP nowait locked\n");
        } else {
            printf("BARBER_FREE_TO_WAKE_UP nowait did not locked\n");
        }
        exit(EXIT_SUCCESS);
    }
    atexit(at_exit);

    wait_semaphore(BARBER_FREE_TO_WAKE_UP);
    printf("BARBER_FREE_TO_WAKE_UP locked\n");
    sleep(4);
    release_semaphore(BARBER_FREE_TO_WAKE_UP);
    printf("BARBER_FREE_TO_WAKE_UP released\n");
}

int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);
    printf("main pid %d\n", getpid());
    removeSemaphores(NAME);
    initSemaphores(NAME);
    simpleTest();
}
