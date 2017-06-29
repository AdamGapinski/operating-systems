#include <stdlib.h>
#include <string.h>

void run(char *command) ;

void default_command() ;

int main(int argc, char **argv) {
    if (argc == 2) {
        run(argv[1]);
    } else {
        default_command();
    }
}

void run(char *command) {
    if (strcmp(command, "time") == 0) {
        while(1);
    }

    if (strcmp(command, "size") == 0) {
        const int SIZE = 100000;
        while(1) {
            long double *arr = calloc(SIZE, sizeof(*arr));
            for(int i = 0; i < SIZE; ++i) {
                arr[i] = 0.0;
            }
        };
    }

    default_command();
}

void default_command() {
    while(1);
}