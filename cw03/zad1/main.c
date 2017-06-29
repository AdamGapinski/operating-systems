#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <unistd.h>
#include <wait.h>
#include <setjmp.h>
#include "scanner.h"

void read_lines(FILE *fp) ;

FILE *open_file(char **filename) ;

void execute_line(char *buff) ;

void assign_env(char *var, token_buff *buff);

void handle_jmp(int jmp, char *line_buff, int line_num) ;

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Error: Wrong number of arguments. Specify file name of batch file\n");
        exit(EXIT_FAILURE);
    }

    FILE *fp = open_file(argv);
    read_lines(fp);

    return 0;
}

jmp_buf jmp_buff;

void read_lines(FILE *fp) {
    size_t line_len = 256;
    int jmp;
    char *line_buff = calloc(line_len, sizeof(*line_buff));

    for (int line_num = 1; getline(&line_buff, &line_len, fp) != -1; ++line_num) {
        /*
         *set jmp may return in two cases:
         *  1. it have just set jmp_buf - saved stack state, then it returns 0 and we should just execute the line.
         *  2. from longjmp(jmp_buff) call and it means that the line has been executed, so we need to check
         *  for errors and then we should go to next line. That is why we have set jmp_buf.
         * */
        if ((jmp = setjmp(jmp_buff)) == 0) {
            execute_line(line_buff);
        } else {
            handle_jmp(jmp, line_buff, line_num);
        }
    }

    free(line_buff);
    fclose(fp);
}

void handle_jmp(int jmp, char *line_buff, int line_num) {
    if (jmp > 0) {
        fprintf(stderr, "%d. line: \"%s\" Exited with failure status\n", line_num, strtok(line_buff, "\n"));
        exit(EXIT_FAILURE);
    } else if (jmp < 0) {
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
    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) != 0){
            longjmp(jmp_buff, WEXITSTATUS(status));
        }
    } else {
        longjmp(jmp_buff, -1);
    }
}

void process(char *filename, token_buff *buff) {
    //create_argv method returns an array of pointers, starting from it' s second index
    char **argv = create_argv(buff);
    argv[0] = filename;

    int pid = fork();
    if (pid == -1) {
        perror("Error while forking");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
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

void execute_line(char *buff) {
    token_buff *token_buff = init_token(buff);
    char *token;

    if ((token = next_token(token_buff)) != NULL) {
        //token is not zero length, otherwise next_token function would return NULL
        if (token[0] == '#') {
            //add + 1 to token to skip # char
            assign_env(strdup(token + 1), token_buff);
        } else {
            process(strdup(token), token_buff);
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