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

void log(char *msg, int slp_flag) ;

int requests_received;
int exp_requests;
int child_processes;

typedef struct proc_hist {
    pid_t pid;
    int *signals;
} proc_hist;

struct processes_info {
    proc_hist **procs;
    int hist_len;
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
    exp_requests = requests;
    child_processes = processes;

    set_signal_handlers();
    fork_processes(processes);
    return 0;
}

void fork_processes(int processes) {
    proc_info.procs = malloc(processes * sizeof(*proc_info.procs));
    proc_info.hist_len = 0;
    const int MAX_SIGNALS_NUM = 10;

    for (int i = 0; i < processes; ++i) {
        int ch;
        if ((ch = fork()) == 0) {
            printf("PID = %d\n", getpid());
            process();
        } else {
            proc_info.procs[i] = malloc(sizeof(*proc_info.procs[i]));
            proc_info.procs[i]->pid = ch;
            proc_info.procs[i]->signals = malloc(MAX_SIGNALS_NUM);
        }
    }
    while (1) {
        //todo test sleep values
        sleep(1);
    }
}

void handle_cont(int signo, siginfo_t *info, void *ptr) {
    return;
}

void process() {
    //setting handling of SIGCONT
    struct sigaction cont_action;
    cont_action.sa_sigaction = handle_cont;
    cont_action.sa_flags |= SA_SIGINFO;
    sigaction(SIGCONT, &cont_action, NULL);

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

    log("sending SIGRTMIN signal as a request", 1);
    if (sigqueue(getppid(), SIGRTMIN, usr_sigval) != 0) {
        perror("Error:");
    }

    printf("PID = %d stopped\n", getpid());
    // todo program receives sigcont before getting to pause
    pause();
    //waiting for response
    printf("PID = %d continued\n", getpid());

    //ending job
    int signal_no = SIGRTMIN + 10;
    union sigval rt_sigval;
    rt_sigval.sival_int = getpid();
    log("sending SIGRTMIN + 10", 1);
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
    sigset_t set;
    sigset_t prev_set;
    sigfillset(&set);
    sigprocmask(SIG_SETMASK, &set, &prev_set);

    printf("%d\n", requests_received);
    ++requests_received;
    if (requests_received > exp_requests) {
    printf("PID = %d received usr1 from %d\n", getpid(), info->si_value.sival_int);
        union sigval sigval;
        printf("PID = %d sending sigcont to %d\n", getpid(), info->si_value.sival_int);
        sleep(5);
        if (sigqueue(info->si_value.sival_int, SIGCONT, sigval) != 0) {
            perror("Error");
        }
        printf("PID = %d sent sigcont to %d\n", getpid(), info->si_value.sival_int);
    } else {
        printf("PID = %d saving %d process request to continue\n", getpid(), info->si_value.sival_int);
        proc_info.procs[proc_info.hist_len]->pid = info->si_value.sival_int;
        proc_info.hist_len ++;
        printf("PID = %d actual requests received %d\n", getpid(), requests_received);
        printf("proc hist len %d\n", proc_info.hist_len);
    }

    if (requests_received == exp_requests) {
        sleep(5);
        for (int i = 0; i < proc_info.hist_len; ++i) {
            union sigval sigval;
            printf("PID = %d sending sigcont to %d\n", getpid(), proc_info.procs[i]->pid);
            if (sigqueue(proc_info.procs[i]->pid, SIGCONT, sigval) != 0) {
                perror("Error");
            }
            printf("PID = %d sent sigcont to %d\n", getpid(), proc_info.procs[i]->pid);
        }
    }

    sigprocmask(SIG_SETMASK, &prev_set, NULL);
}

void handle_child_exit(int signo, siginfo_t *info, void *ptr) {
    sigset_t set;
    sigset_t prev_set;
    sigfillset(&set);
    sigprocmask(SIG_SETMASK, &set, &prev_set);


    printf("%d received rt sig from %d\n", getpid(), info->si_value.sival_int);
    --child_processes;
    printf("%d subtracting to %d\n", getpid(), child_processes);
    if (child_processes == 0) {
        int tmp = 0;
        for (int i = 0; i < proc_info.hist_len; ++i) {
            int status;
            int pid = proc_info.procs[i]->pid;
            waitpid(pid, &status, 0);
            printf("%d exited with status %d\n", pid, status);
        }
        exit(EXIT_SUCCESS);
    }

    sigprocmask(SIG_SETMASK, &prev_set, NULL);
}

void set_signal_handlers() {
    //This will set handler for SIGUSR1 signals treated as requests to continue work made by child processes
    struct sigaction usr_action;
    usr_action.sa_sigaction = handle_request;
    usr_action.sa_flags |= SA_SIGINFO;
    //usr_action.sa_flags |= SA_NODEFER;
    sigaction(SIGRTMIN, &usr_action, NULL);

    struct sigaction rt_action;
    rt_action.sa_sigaction = handle_child_exit;
    rt_action.sa_flags |= SA_SIGINFO;

    //todo rt_set may be redundant
    sigset_t rt_set;
    sigemptyset(&rt_set);
    int sig_no = SIGRTMIN + 1;
    while (sig_no <= SIGRTMAX) {
        sigaction(sig_no, &rt_action, NULL);
        sigaddset(&rt_set, sig_no);
        sig_no++;
    }
}

void log(char *msg, int slp_flag) {
    printf("%d\t%s\t%d\n", getpid(), msg, getppid());
    if (slp_flag == 1) {
        sleep(10);
    }
}