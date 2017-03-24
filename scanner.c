#include <stdlib.h>
#include <memory.h>
#include "scanner.h"

token_buff *init_token(char *buff) {
    token_buff *token = malloc(sizeof(*token));
    token->buff = buff;
    token->pointer = buff;
    token->token = NULL;
    token->spaces_before = 0;
    return token;
}

void remove_token(token_buff *token) {
    free(token->token);
    free(token);
}

size_t skip_word(char **line) {
    size_t word_len = 0;
    while(**line != ' ' && **line != '\n' && **line != '\0') {
        *line += 1;
        word_len += 1;
    }
    return word_len;
}

int skip_blank(char **line) {
    int blanks = 0;
    while(**line != '\0') {
        if (**line == ' ' || **line == '\n') {
            *line += 1;
            blanks += 1;
        } else {
            return blanks;
        }
    }
    //subtract 1 because of \0 character
    return blanks == 0 ? blanks : blanks - 1;
}

char *next_token(token_buff *token_buff) {
    free(token_buff->token);
    token_buff->spaces_before = skip_blank(&token_buff->pointer);

    char *prev_pointer = token_buff->pointer;
    size_t token_len;
    if ((token_len = skip_word(&token_buff->pointer)) != 0) {
        token_buff->token = strndup(prev_pointer, token_len);
    } else {
        token_buff->token = NULL;
    }

    return token_buff->token;
}