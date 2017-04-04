#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>

void send_signals(int signals, int type, int process_id) ;

void set_handlers_process();

void set_handlers();

void print(char *msg, int value) {
    char *msg_cpy = calloc(500, sizeof(*msg_cpy));
    sprintf(msg_cpy, "%s\t%d\n", msg, value);
    write(STDOUT_FILENO, msg_cpy, strlen(msg_cpy));
    free(msg_cpy);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
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
        set_handlers_process();
        while(1) sleep(100);
    } else {
        set_handlers();
        send_signals(signals, type, process_id);
    }
    return 0;
}

int received;

void handle_usr (int signo, siginfo_t *info, void *ptr) {
    if (signo == SIGUSR1) {
        ++received;
    } else if (signo == SIGUSR2) {
        print("signals received by parent process", received);
        _exit(EXIT_SUCCESS);
    }
}

void handle_rtmin (int signo, siginfo_t *info, void *ptr) {
    if (signo == SIGRTMIN) {
        ++received;
    } else if (signo == SIGRTMIN + 1) {
        print("signals received by parent process", received);
        _exit(EXIT_SUCCESS);
    }
}

void set_handlers() {
    struct sigaction usr_action;
    usr_action.sa_sigaction = handle_usr;
    sigaction(SIGUSR1, &usr_action, NULL);
    sigaction(SIGUSR2, &usr_action, NULL);

    struct sigaction rt_min;
    rt_min.sa_sigaction = handle_rtmin;
    sigaction(SIGRTMIN, &rt_min, NULL);
    sigaction(SIGRTMIN + 1, &rt_min, NULL);
}

int received_by_process;

void handle_usr_process(int signo, siginfo_t *info, void *ptr) {
    if (signo == SIGUSR1) {
        ++received_by_process;
        kill(getppid(), SIGUSR1);
    } else if (signo == SIGUSR2) {
        print("signals received by child process", received_by_process);
        kill(getppid(), SIGUSR2);
        _exit(EXIT_SUCCESS);
    }
}

void handle_rtmin_process(int signo, siginfo_t *info, void *ptr) {
    if (signo == SIGRTMIN) {
        ++received_by_process;
        kill(getppid(), SIGRTMIN);
    } else if (signo == SIGRTMIN + 1) {
        print("signals received by child process", received_by_process);
        kill(getppid(), SIGRTMIN + 1);
        _exit(EXIT_SUCCESS);
    }
}

void set_handlers_process() {
    struct sigaction usr_action;
    usr_action.sa_sigaction = handle_usr_process;
    sigaction(SIGUSR1, &usr_action, NULL);
    sigaction(SIGUSR2, &usr_action, NULL);

    struct sigaction rt_min;
    rt_min.sa_sigaction = handle_rtmin_process;
    sigaction(SIGRTMIN, &rt_min, NULL);
    sigaction(SIGRTMIN + 1, &rt_min, NULL);
}

void send_signals(int signals, int type, int process_id) {
    sleep(1);
    if (type == 1) {
        for (int i = 0; i < signals; ++i) {
            kill(process_id, SIGUSR1);
        }
        printf("Parent process sent %d signals by kill method\n", signals);
        kill(process_id, SIGUSR2);
    } else if (type == 2) {
        union sigval val;
        for (int i = 0; i < signals; ++i) {
            sigqueue(process_id, SIGUSR1, val);
        }
        printf("Parent process sent %d signals by sigqueue method\n", signals);
        sigqueue(process_id, SIGUSR2, val);
    } else if (type == 3) {
        for (int i = 0; i < signals; ++i) {
            kill(process_id, SIGRTMIN);
        }
        kill(process_id, SIGRTMIN + 1);
        printf("Parent process sent %d real time signals\n", signals);
    } else {
        return;
    }
    //waiting for child process response
    while(1) sleep(100);
}
