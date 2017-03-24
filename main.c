#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include "scanner.h"

void read_lines(FILE *fp) ;

FILE *open_file(char **filename) ;

void parse_line(char *buff) ;

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

    while (next_token(token_buff) != NULL) {
        printf("%s\n", token_buff->token);
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
