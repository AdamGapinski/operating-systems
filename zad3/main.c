#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>

void send_signals(int signals, int type, int process_id) ;

void set_process_handlers();

void log(char *msg, int parent_id, int pid, int slp_flag) {
    char *msg_cpy = calloc(500, sizeof(*msg_cpy));
    sprintf(msg_cpy, "%d\t%s\t%d\n", parent_id, msg, pid);

    write(STDOUT_FILENO, msg_cpy, strlen(msg_cpy));
    if (slp_flag == 1) {
        sleep(5);
    }
    free(msg_cpy);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error: Wrong number of arguments. Specify two arguments: L - number of signals and Type (1, 2, 3)\n");
        exit(EXIT_FAILURE);
    }

    int signals;
    if (sscanf(argv[1], "%d", &signals) != 1) {
        fprintf(stderr, "Error: While parsing L - number of signals\n");
        exit(EXIT_FAILURE);
    }

    int type;
    if (sscanf(argv[2], "%d", &type) != 1 || (type != 1 && type != 2 && type != 3)) {
        fprintf(stderr, "Error: While parsing Type\n");
        exit(EXIT_FAILURE);
    }

    int process_id;
    if ((process_id = fork()) == 0) {
        set_process_handlers();
        while(1);
    } else {
        set_hanlders();
        send_signals(signals, type, process_id);
    }
    return 0;
}

void set_process_handlers() {
    struct sigaction usr_action;
    usr_action.sa_sigaction = handle_usr1;
    sigaction(SIGUSR1, &usr_action, NULL);

    struct sigaction usr_action;
    usr_action.sa_sigaction = handle_usr1;
    sigaction(SIGUSR1, &usr_action, NULL);
}

void send_signals(int signals, int type, int process_id) {
    if (type == 1) {
        for (int i = 0; i < signals; ++i) {
            kill(process_id, SIGUSR1);
        }

        kill(process_id, SIGUSR2);
    } else if (type == 2) {
        for (int i = 0; i < signals; ++i) {
            sigqueue(process_id, SIGUSR1, NULL);
        }

        sigqueue(process_id, SIGUSR2, NULL);
    } else if (type == 3) {
        for (int i = 0; i < signals; ++i) {
            kill(process_id, SIGRTMIN);
        }

        kill(process_id, SIGRTMIN + 1);
    }
}
