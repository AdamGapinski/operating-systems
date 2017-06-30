#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/user.h>
#include "../include/SharedQueue.h"

char *pathname = "/home/adam";
int proj_id = 215;

void test_shared_variables();

void test_shared_array();

void simpleTest() {
    test_shared_variables();
    test_shared_array();
}

void test_shared_array() {
    void *shared = get_shared_address(6 * sizeof(int), pathname, proj_id + 1);
    int *shared_value = shared;
    int *shared_value1 = shared + sizeof(*shared_value);
    int *shared_value2 = shared + sizeof(*shared_value) + sizeof(*shared_value1);
    int *shared_array = shared + sizeof(*shared_value) + sizeof(*shared_value1) + sizeof(*shared_value2);

    *shared_value = 0;
    *shared_value1 = 1;
    *shared_value2 = 2;

    shared_array[0] = 123;
    shared_array[1] = 124;
    shared_array[2] = 125;

    if (fork() == 0) {
        void *forked_shared = get_shared_address(1, pathname, proj_id + 1);
        int *forked_shared_value = forked_shared;
        int *forked_shared_value1 = forked_shared + sizeof(*shared_value);
        int *forked_shared_value2 = forked_shared + sizeof(*shared_value) + sizeof(*shared_value1);
        int *forked_shared_array = forked_shared + sizeof(*shared_value) + sizeof(*shared_value1) + sizeof(*shared_value2);

        forked_shared_array[0] = 122;
        forked_shared_array[1] = 133;
        forked_shared_array[2] = 144;

        *forked_shared_value = 12;
        *forked_shared_value1 = 13;
        *forked_shared_value2 = 14;
        exit(EXIT_SUCCESS);
    }

    sleep(2);
    printf("shared value changed to %d\n", *shared_value);
    printf("shared value changed1 to %d\n", *shared_value1);
    printf("shared value changed2 to %d\n", *shared_value2);

    printf("shared array[0] changed to %d\n", shared_array[0]);
    printf("shared array[1] changed1 to %d\n", shared_array[1]);
    printf("shared array[2] changed2 to %d\n", shared_array[2]);

    remove_shared_address(pathname, proj_id + 1, shared);
}

void test_shared_variables() {
    void *shared = get_shared_address(6 * sizeof(int), pathname, proj_id);
    int *shared_value = shared;
    int *shared_value1 = shared + sizeof(*shared_value);
    int *shared_value2 = shared + sizeof(*shared_value) + sizeof(*shared_value1);

    *shared_value = 0;
    *shared_value1 = 1;
    *shared_value2 = 2;

    if (fork() == 0) {
        printf("forked process changing value of shared_value to 12\n");
        void *forked_shared = get_shared_address(1, pathname, proj_id);
        int *forked_shared_value = forked_shared;
        int *forked_shared_value1 = forked_shared + sizeof(*forked_shared_value);
        int *forked_shared_value2 = forked_shared + 2 * sizeof(*forked_shared_value1);

        *forked_shared_value = 12;
        *forked_shared_value1 = 13;
        *forked_shared_value2 = 14;
        exit(EXIT_SUCCESS);
    }

    sleep(2);
    printf("shared value changed to %d\n", *shared_value);
    printf("shared value changed1 to %d\n", *shared_value1);
    printf("shared value changed2 to %d\n", *shared_value2);
    remove_shared_address(pathname, proj_id, shared);
}

int main(int argc, char *argv[]) {
    simpleTest();
    initQueue(pathname, 2, proj_id);
    printf("");
}


