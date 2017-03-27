#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <wait.h>
#include <setjmp.h>
#include <unistd.h>
#include <sys/resource.h>
#include <float.h>
#include "scanner.h"

void read_lines(FILE *fp, rlim_t time_limit, rlim_t size_limit) ;

FILE *open_file(char **filename) ;

void execute_line(char *buff, rlim_t time_limit, rlim_t size_limit) ;

void assign_env(char *var, token_buff *buff);

void handle_jmp(int jmp, char *line_buff, int line_num) ;

long parse_limit(char *string);

void set_limits(rlim_t time_limit, rlim_t size_limit) ;

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Error: Wrong number of arguments. %s %s\n",
                "Specify file name of batch file, CPU time limit (in seconds)",
                "and memory limit (in megabytes).");
        exit(EXIT_FAILURE);
    }

    long time_limit = parse_limit(argv[2]);
    long size_limit = parse_limit(argv[3]);

    if (time_limit < 0 || size_limit < 0) {
        fprintf(stderr, "Error: Limit cannot be below zero\n");
        exit(EXIT_FAILURE);
    }

    FILE *fp = open_file(argv);
    read_lines(fp, (rlim_t) time_limit, (rlim_t) size_limit);

    return 0;
}

long parse_limit(char *string) {
    long result = 0;
    char *endptr;
    result = strtol(string, &endptr, 10);

    if (endptr == string || strcmp(endptr, "") != 0) {
        fprintf(stderr, "Error: limit argument has to be a number\n");
        exit(EXIT_FAILURE);
    }

    return result;
}

jmp_buf jmp_buff;

void read_lines(FILE *fp, rlim_t time_limit, rlim_t size_limit) {
    size_t line_len = 256;
    int jmp;
    char *line_buff = calloc(line_len, sizeof(*line_buff));

    for (int line_num = 1; getline(&line_buff, &line_len, fp) != -1; ++line_num) {
        /*
         *set jmp may return in two cases:
         *  1. it have just set jmp_buf - saved stack state, then it returns 0 and we should just execute the line.
         *  2. from longjmp(jmp_buff) call and it means that the line has been executed, so we need to summarize it
         *     and then we should go to next line. That is why we have set jmp_buf.
         * */
        if ((jmp = setjmp(jmp_buff)) == 0) {
            execute_line(line_buff, time_limit, size_limit);
        } else {
            handle_jmp(jmp, line_buff, line_num);
        }
    }

    free(line_buff);
    fclose(fp);
}

struct usge {
    double utime, stime;
    long minflst, majflt, inblock, oublock, nvcsw, nivcsw;
} total_usage;

double get_diff_dbl(double *prev, double actual) {
    double result;
    result = actual - *prev;
    *prev = actual;
    return result;
}

long get_diff_lng(long *prev, long actual) {
    long result;
    result = actual - *prev;
    *prev = actual;
    return result;
}

void report_resource_usage(char *line_buff, int line_num) {
    struct rusage usage;
    getrusage(RUSAGE_CHILDREN, &usage);
    double SEC_TO_MICRO = 1000000;

    double utime = get_diff_dbl(&total_usage.utime, usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / SEC_TO_MICRO);
    double stime = get_diff_dbl(&total_usage.stime, usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / SEC_TO_MICRO);

    long minflst = get_diff_lng(&total_usage.minflst, usage.ru_minflt);
    long majflt = get_diff_lng(&total_usage.majflt, usage.ru_majflt);
    long inblock = get_diff_lng(&total_usage.inblock, usage.ru_inblock);
    long oublock = get_diff_lng(&total_usage.oublock, usage.ru_oublock);
    long nvcsw = get_diff_lng(&total_usage.nvcsw, usage.ru_nvcsw);
    long nivcsw = get_diff_lng(&total_usage.nivcsw, usage.ru_nivcsw);

    printf("line %d\t\"%s\" executed in\t user: %.6fs\t system: %.6fs\tblock\n", line_num, strtok(line_buff, "\n"), utime, stime);
}

void handle_jmp(int jmp, char *line_buff, int line_num) {
    if (jmp == 100) {
        //jmp equal to 100 means that, the line has been executed and we can report the process resource usage statistics
        report_resource_usage(line_buff, line_num);
    } else if (jmp == SIGXCPU || jmp == SIGKILL) {
        fprintf(stderr, "%d. line: \"%s\" Error: CPU time limit exceeded\n", line_num, strtok(line_buff, "\n"));
        exit(EXIT_FAILURE);
    } else if (jmp == SIGSEGV) {
        fprintf(stderr, "%d. line: \"%s\" Error: Segmentation fault\n", line_num, strtok(line_buff, "\n"));
        exit(EXIT_FAILURE);
    } else if (jmp == EXIT_FAILURE) {
        fprintf(stderr, "%d. line: \"%s\" Exited with failure status\n", line_num, strtok(line_buff, "\n"));
        exit(EXIT_FAILURE);
    } else {
        fprintf(stderr, "%d. line: \"%s\" Unexpected error occurred\n", line_num, strtok(line_buff, "\n"));
        exit(EXIT_FAILURE);
    }
}

//this method will leave the first element of the returned array free
char **create_argv(token_buff *buff) {
    const int DEF_ARG_NUM = 25;
    char **argv = calloc(DEF_ARG_NUM, sizeof(*argv));

    char *token;
    for (int i = 1; (token = next_token(buff)) != NULL; ++i) {
        argv[i] = strdup(token);
    }

    return argv;
}

void remove_argv(char **argv) {
    for (int i = 0; argv[i] != NULL; ++i) {
        free(argv[i]);
    }
    free(argv);
}

void check_error(int status) {
    //jump handling will be made in handle_jump call in the read_lines method
    if (WIFEXITED(status)) {
        //This is true if the process has exited from main return or exit method call.
        if (WEXITSTATUS(status) == 0) {
            //longjmp cannot cause 0 to be returned. Instead it would return 1 and could be
            //mistaken with SIGHUP signal
            longjmp(jmp_buff, 100);
        }
        longjmp(jmp_buff, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        //The process has terminated from signal
        longjmp(jmp_buff, WTERMSIG(status));
    }

    //unexpected error
    longjmp(jmp_buff, -1);
}

void process(char *filename, token_buff *buff, rlim_t time_limit, rlim_t size_limit) {
    //create_argv method returns an array of pointers, starting from it' s second index
    char **argv = create_argv(buff);
    argv[0] = filename;

    int pid = fork();
    if (pid == -1) {
        perror("Error while forking");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        set_limits(time_limit, size_limit);

        //execvp is variety of exec that takes it' s arguments as an array of pointers and
        //takes filename of the executable file and search for it in the directories specified by PATH env variable.
        if (execvp(filename, argv) == 0) {
            exit(EXIT_SUCCESS);
        } else {
            perror("Error while executing file");
            exit(EXIT_FAILURE);
        }
    } else {
        int stat = 0;
        wait(&stat);
        //we do not have to free filename, because pointer to filename is in the argv[0]
        remove_argv(argv);
        check_error(stat);
        //never returns
    }
}

void set_limits(rlim_t time_limit, rlim_t size_limit) {
    const int MEGABYTES_TO_BYTES = 1000000;
    struct rlimit *limit = malloc(sizeof(*limit));

    limit->rlim_cur = limit->rlim_max = time_limit;
    if (setrlimit(RLIMIT_CPU, limit) != 0) {
        perror("Error");
        exit(EXIT_FAILURE);
    }

    limit->rlim_cur = limit->rlim_max = (rlim_t) (size_limit * MEGABYTES_TO_BYTES);
    if (setrlimit(RLIMIT_AS, limit) != 0){
        perror("Error");
        exit(EXIT_FAILURE);
    }
    free(limit);
}

void execute_line(char *buff, rlim_t time_limit, rlim_t size_limit) {
    token_buff *token_buff = init_token(buff);
    char *token;

    if ((token = next_token(token_buff)) != NULL) {
        //token is not zero length, otherwise next_token function would return NULL
        if (token[0] == '#') {
            //add + 1 to token to skip # char
            assign_env(strdup(token + 1), token_buff);
        } else {
            process(strdup(token), token_buff, time_limit, size_limit);
        }
    }
}

FILE *open_file(char **filename) {
    FILE *fp;
    if ((fp = fopen(filename[1], "r")) == NULL) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return fp;
}

const size_t BASIC_VAL_LEN = 256;

const size_t PROLONG_VAL_LEN = 100;

void check_length(char **valueP, int index, int *reallocs) {
    char *value = *valueP;
    if (index + 1 == BASIC_VAL_LEN + *reallocs * PROLONG_VAL_LEN) {
        *valueP = realloc(value, BASIC_VAL_LEN + ++(*reallocs) * PROLONG_VAL_LEN);
    }
}

void add_spaces(char **value, int spaces, int *index, int *reallocs) {
    for (int i = 0; i < spaces; ++i) {
        check_length(value, *index, reallocs);
        (*value)[(*index)++] = ' ';
    }
}

void assign_env(char *var, token_buff *buff) {
    char *token = next_token(buff);
    if (token == NULL) {
        unsetenv(var);
        free(var);
        return;
    }

    char *value = calloc(BASIC_VAL_LEN, sizeof(*value));
    int reallocs = 0;
    int index = 0;
    do {
        for (int i = 0; token[i] != '\0'; ++i) {
            check_length(&value, index, &reallocs);
            value[index++] = token[i];
        }

        token = next_token(buff);
        if (token != NULL) {
            add_spaces(&value, buff->spaces_before, &index, &reallocs);
        }
    } while (token != NULL);

    //no index out of bound because of check_length calls, which prolongs value array,
    //when index is pointing to the last element of array.
    value[index] = '\0';

    //last argument 1 for overwrite
    setenv(var, value, 1);
    free(value);
    free(var);
}
