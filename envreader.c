#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Error: Wrong number of arguments. Specify environment variable name\n");
        exit(EXIT_FAILURE);
    }

    char *env = getenv(argv[1]);

    if (env == NULL) {
        printf("%s environment variable does not exists.\n", argv[1]);
    } else {
        printf("%s=%s\n", argv[1], env);
    }

    return 0;
}