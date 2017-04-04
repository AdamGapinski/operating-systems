#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <wait.h>
#include <string.h>

void fork_processes(int processes) ;

void set_signal_handlers() ;

void process(int parent) ;

void set_rt_signals();

void set_usr1();

void set_sigint();

volatile int requests_received;
volatile int child_processes;
int EXP_REQUESTS;

typedef struct proc_hist {
    volatile pid_t pid;
    volatile int *signals;
    volatile int sig_count;
} proc_hist;

struct processes_info {
    volatile proc_hist **procs;
    volatile int hist_len;
} proc_info;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Error: Wrong number of arguments. Specify two arguments: N - processes and K - requests\n");
        exit(EXIT_FAILURE);
    }
    int processes, requests;
    if (sscanf(argv[1], "%d", &processes) != 1) {
        fprintf(stderr, "Error: While parsing N - processes number\n");
        exit(EXIT_FAILURE);
    }
    if (sscanf(argv[2], "%d", &requests) != 1) {
        fprintf(stderr, "Error: While parsing K - requests number\n");
        exit(EXIT_FAILURE);
    }
    if (requests > processes) {
        fprintf(stderr, "Error: Number of requests cannot be greater than number of processes\n");
        exit(EXIT_FAILURE);
    }
    EXP_REQUESTS = requests;
    child_processes = processes;

    set_signal_handlers();
    fork_processes(processes);
    return 0;
}

void fork_processes(int processes) {
    proc_info.procs = malloc(processes * sizeof(*proc_info.procs));
    proc_info.hist_len = 0;
    const int MAX_SIGNALS_NUM = 10;

    int parent = getpid();
    for (int i = 0; i < processes; ++i) {
        if (fork() == 0) {
            process(parent);
        } else {
            proc_info.procs[i] = malloc(sizeof(*proc_info.procs[i]));
            proc_info.procs[i]->signals = malloc(MAX_SIGNALS_NUM);
            proc_info.procs[i]->sig_count = 0;
        }
    }
    while (1) {
        sleep(1);
        if (child_processes == 0) {
            for (int i = 0; i < proc_info.hist_len; ++i) {
                volatile proc_hist *info = proc_info.procs[i];
                int status = 0;
                int pid = info->pid;
                int sig_count = info->sig_count;

                waitpid(pid, &status, 0);
                printf("\tProcess id %d\texit status %d\t%d signals:\n", pid, WEXITSTATUS(status), sig_count);
                for (int j = 0; j < info->sig_count; ++j) {
                    printf("\t\t%s", strsignal(info->signals[j]));
                }
                printf("\n\n");
            }
            exit(EXIT_SUCCESS);
        }
    }
}

void process(int parent) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR2);
    sigprocmask(SIG_BLOCK, &set, NULL);

    srand48(time(NULL) ^ (getpid()<<16));
    int sleep_time = (int) (drand48() * 11);
    sleep((unsigned int) sleep_time);

    //sending request
    union sigval usr_sigval;
    usr_sigval.sival_int = getpid();
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);

    if (sigqueue(getppid(), SIGUSR1, usr_sigval) != 0) {
        perror("Error:");
    }

    struct timespec time;
    time.tv_sec = 11;
    time.tv_nsec = 0;

    //waiting for response
    if (sigtimedwait(&set, NULL, &time) == -1) {
        child_processes--;
        exit(100);
    }


    int signal_no = SIGRTMIN + (int) (drand48() * (SIGRTMAX - SIGRTMIN + 1));
    union sigval rt_sigval;
    rt_sigval.sival_int = getpid();

    if (parent == getppid()) {
        if (sigqueue(getppid(), signal_no, rt_sigval) != 0) {
            perror("Error");
        }
    }

    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);

    //exiting with time diff in seconds
    exit((int) (end.tv_sec - start.tv_sec));
}

void handle_request(int signo, siginfo_t *info, void *ptr) {
    ++requests_received;

    if (requests_received > EXP_REQUESTS) {
        if (kill(info->si_value.sival_int, SIGUSR2) != 0) {
            perror("Error");
        }
    }
    int index = proc_info.hist_len;
    proc_info.procs[index]->pid = info->si_value.sival_int;
    proc_info.procs[index]->signals[proc_info.procs[index]->sig_count] = signo;

    ++proc_info.procs[index]->sig_count;
    ++proc_info.hist_len;

    if (requests_received == EXP_REQUESTS) {
        for (int i = 0; i < proc_info.hist_len; ++i) {
            if (kill(proc_info.procs[i]->pid, SIGUSR2) != 0) {
                perror("Error");
            }
        }
    }

    set_usr1();
}

void handle_child_exit(int signo, siginfo_t *info, void *ptr) {
    --child_processes;
    for (int i = 0; i < proc_info.hist_len; ++i) {
        volatile proc_hist *p_info = proc_info.procs[i];
        if (p_info->pid == info->si_value.sival_int) {
            p_info->signals[p_info->sig_count] = signo;
            ++p_info->sig_count;
        }
    }

    set_rt_signals();
}

void handle_int(int signo, siginfo_t *info, void *ptr) {
    for (int i = 0; i < proc_info.hist_len; ++i) {
        kill(proc_info.procs[i]->pid, SIGKILL);
    }
    raise(SIGKILL);
}

void set_signal_handlers() {
    set_sigint();
    set_usr1();
    set_rt_signals();
}

void set_sigint() {
    struct sigaction int_action;
    int_action.sa_sigaction = handle_int;
    sigaction(SIGINT, &int_action, NULL);
}

void set_usr1() {//This will set handler for SIGUSR1 signals treated as requests to continue child processes
    struct sigaction usr_action;
    usr_action.sa_sigaction = handle_request;
    usr_action.sa_flags |= SA_SIGINFO;
    sigaction(SIGUSR1, &usr_action, NULL);
}

void set_rt_signals() {
    struct sigaction rt_action;
    rt_action.sa_sigaction = handle_child_exit;
    rt_action.sa_flags |= SA_SIGINFO;
    for (int i = SIGRTMIN; i <= SIGRTMAX; ++i) {
        sigaction(i, &rt_action, NULL);
    }
}