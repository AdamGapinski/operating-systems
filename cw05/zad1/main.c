#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <wait.h>

void execute_line(char *line_ptr, size_t len);

void remove_argv(char **argv) ;

int fork_run(char *program, char **argv, int in, int i);

char **create_argv(char *line) ;

int run_command(char *line, int in, int last) ;

int main() {
    size_t line_len = 0;
    char *line_ptr = NULL;

    while(getline(&line_ptr, &line_len, stdin) != -1) {
        execute_line(line_ptr, line_len);
    }

    if (errno == EINVAL) {
        perror("Line reading error");
    }
    free(line_ptr);
    exit(EXIT_SUCCESS);
}

void execute_line(char *line_ptr, size_t len) {
    char **saveptr = &line_ptr;
    char *token = strtok_r(line_ptr, "|\n", saveptr);
    int prev_out = -1;
    char *next_token;
    while (token != NULL) {
        next_token = strtok_r(NULL, "|\n", saveptr);

        if (next_token != NULL) {
            prev_out = run_command(token, prev_out, 0);
        } else {
            run_command(token, prev_out, 1);
        }

        token = next_token;
    }

    int status = 0;
    while(waitpid(-1, &status, 0) > 0) {
        if (WEXITSTATUS(status) == EXIT_FAILURE) {
            exit(EXIT_FAILURE);
        }
    };
}

int run_command(char *line, int in, int last) {
    if (line == NULL || strlen(line) == 0) return -1;
    char **argv = create_argv(line);

    int out = fork_run(argv[0], argv, in, last);

    remove_argv(argv);
    return out;
}

int fork_run(char *program, char **argv, int in, int last) {
    int pfd[2];
    pipe(pfd);

    if (fork() == 0) {
        close(pfd[0]);
        if (pfd[1] != STDOUT_FILENO) {
            if (last == 0) {

                dup2(pfd[1], STDOUT_FILENO);
            }
            close(pfd[1]);
        }

        if (in != STDIN_FILENO) {
            if (in != -1) {
                dup2(in, STDIN_FILENO);
            } else {
                close(STDIN_FILENO);
            }
            close(in);
        }

        if (execvp(program, argv) == -1) {
            fprintf(stderr, "Error while running program %s: %s\n", program, strerror(errno));
            exit(EXIT_FAILURE);
        }
        //execvp never returns on success
    }

    close(pfd[1]);
    return pfd[0];
}

char **create_argv(char *line) {
    const int MAX_ARG_NUM = 25;
    char **argv = calloc(MAX_ARG_NUM, sizeof(*argv));
    char **saveptr = &line;
    char *token = strtok_r(line, " \n", saveptr);

    int index = 0;
    do {
        argv[index] = strdup(token);
        ++index;
    } while((token = strtok_r(NULL, " \n", saveptr)) != NULL);

    return argv;
}

void remove_argv(char **argv) {
    for (int i = 0; argv[i] != NULL; ++i) {
        free(argv[i]);
    }
    free(argv);
}
