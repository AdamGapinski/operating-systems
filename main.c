#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include "scanner.h"

void read_lines(FILE *fp) ;

FILE *open_file(char **filename) ;

void parse_line(char *buff) ;

void assign_env(char *var, token_buff *buff);

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Error: Wrong number of arguments specified. Specify batch program file name\n");
        exit(EXIT_FAILURE);
    }

    FILE *fp = open_file(argv);
    read_lines(fp);

    return 0;
}

void read_lines(FILE *fp) {
    size_t line_len = 256;
    char *line_buff = calloc(line_len, sizeof(*line_buff));

    while (getline(&line_buff, &line_len, fp) != -1) {
        parse_line(line_buff);
    }

    free(line_buff);
}

void parse_line(char *buff) {
    token_buff *token_buff = init_token(buff);

    char *token;
    while ((token = next_token(token_buff)) != NULL) {
        //token is not zero length, otherwise next_token function would return NULL
        if (token[0] == '#') {
            //add + 1 to token to skip # char
            assign_env(strdup(token + 1), token_buff);
        } else {

        }
    }

    remove_token(token_buff);
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
}
