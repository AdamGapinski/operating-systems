#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <bits/siginfo.h>
#include <wait.h>

void fork_processes(int processes) ;

void set_signal_handlers() ;

void process();

int requests_received;
int exp_requests;
int child_processes;

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
    exp_requests = requests;
    child_processes = processes;

    set_signal_handlers();
    fork_processes(processes);
    return 0;
}

void fork_processes(int processes) {
    for (int i = 0; i < processes; ++i) {
        if (fork() == 0) {
            printf("PID = %d\n", getpid());
            process();
        }
    }
    while (1) {
        //todo test sleep values
        sleep(1);
    }
}


void process() {
    //todo set time randomization
    srand48(time(NULL));
    int sleep_time = (int) (drand48() * 11);
    sleep((unsigned int) sleep_time);
    printf("PID = %d, randomized %d time\n", getpid(), sleep_time);

    //sending request
    union sigval usr_sigval;
    usr_sigval.sival_int = getpid();
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    printf("PID = %d send usr1 sig\n", getpid());
    if (sigqueue(getppid(), SIGUSR1, usr_sigval) != 0) {
        perror("Error:");
    }

    printf("PID = %d stopped\n", getpid());
    //waiting for response
    raise(SIGSTOP);
    printf("PID = %d continued\n", getpid());

    //ending job
    int signal_no = SIGRTMIN + (int) (drand48() * (SIGRTMAX - SIGRTMIN + 1));
    union sigval rt_sigval;
    rt_sigval.sival_int = getpid();
    printf("PID = %d send real time sig\n", getpid());
    if (sigqueue(getppid(), signal_no, rt_sigval) != 0) {
        perror("Error");
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    struct timespec end;

    //exiting with time diff in seconds
    printf("PID = %d ending\n", getpid());
    exit((int) (end.tv_sec - start.tv_sec));
}

void handle_request(int signo, siginfo_t *info, void *ptr) {
    ++requests_received;
    if (requests_received >= exp_requests) {
    printf("PID = %d received usr1 from %d\n", getpid(), info->si_value.sival_int);
        union sigval sigval;
        printf("PID = %d sending sigcont to %d\n", getpid(), info->si_value.sival_int);
        if (sigqueue(info->si_value.sival_int, SIGCONT, sigval) != 0) {
            perror("Error");
        }
        printf("PID = %d sent sigcont to %d\n", getpid(), info->si_value.sival_int);
    }
}

void handle_child_exit(int signo, siginfo_t *info, void *ptr) {
    printf("PID = %d received rt sig from %d\n", getpid(), info->si_value.sival_int);
    --child_processes;
    printf("PID = %d subtracting to %d\n", getpid(), child_processes);
    if (child_processes == 0) {
        wait(NULL);
        printf("PID = %d waited for %d\n", getpid(), info->si_value.sival_int);
        exit(EXIT_SUCCESS);
    }
}

void set_signal_handlers() {
    //This will set handler for SIGUSR1 signals treated as requests to continue work made by child processes
    struct sigaction usr_action;
    usr_action.sa_sigaction = handle_request;
    usr_action.sa_flags |= SA_SIGINFO;
    //usr_action.sa_flags |= SA_NODEFER;
    sigaction(SIGUSR1, &usr_action, NULL);

    struct sigaction rt_action;
    rt_action.sa_sigaction = handle_child_exit;
    rt_action.sa_flags |= SA_SIGINFO;

    //todo rt_set may be redundant
    sigset_t rt_set;
    sigemptyset(&rt_set);
    int sig_no = SIGRTMIN;
    while (sig_no <= SIGRTMAX) {
        sigaction(sig_no, &rt_action, NULL);
        sigaddset(&rt_set, sig_no);
        sig_no++;
    }
}