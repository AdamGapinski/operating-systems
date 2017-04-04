#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>

void log(char *msg, int parent_id, int pid, int slp_flag) {
    char *msg_cpy = calloc(500, sizeof(*msg_cpy));
    sprintf(msg_cpy, "%d\t%s\t%d\n", parent_id, msg, pid);

    write(STDOUT_FILENO, msg_cpy, strlen(msg_cpy));
    if (slp_flag == 1) {
        sleep(5);
    }
    free(msg_cpy);
}

int main() {
    log("test", 1,1 ,0);
    return 0;
}
