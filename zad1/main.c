#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>

char direction = 'F';

void set_signal_handlers();

void print_alphabet();

int main() {
    set_signal_handlers();
    print_alphabet();

    return 0;
}

void interrupt_handler(int signo) {
    write(STDOUT_FILENO, "Odebrano sygnal SIGINT\n", 23);
    _exit(signo);
}

void typed_stop_handler(int signo, siginfo_t *info, void *ptr) {
    if (direction == 'F') {
        direction = 'B';
    } else if (direction == 'B') {
        direction = 'F';
    }
}

void print_alphabet() {
    char a = 'A';
    int offset = 0;
    while(1) {
        printf("%c\n", a + offset);

        if (direction == 'F') {
            offset += 1;
        } else if (direction == 'B') {
            offset -= 1;
        }

        if (a + offset - 1 == 'Z') {
            offset = 0;
        } else if (a + offset + 1 == 'A') {
            offset = 'Z' - 'A';
        }
        sleep(1);
    }
}

void set_signal_handlers() {
    signal(SIGINT, interrupt_handler);
    struct sigaction action;
    action.sa_sigaction = typed_stop_handler;
    sigaction(SIGTSTP, &action, NULL);
}