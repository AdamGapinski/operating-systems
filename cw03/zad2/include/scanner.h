#ifndef PROCESY_ZESTAW3_SCANNER_H
#define PROCESY_ZESTAW3_SCANNER_H

typedef struct token_buff {
    char *buff;
    char *pointer;
    char *token;
    int spaces_before;
} token_buff;

token_buff *init_token(char *buff);
void remove_token(token_buff *token);
char *next_token(token_buff *token_buff);

#endif //PROCESY_ZESTAW3_SCANNER_H
