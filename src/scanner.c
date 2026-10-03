#include "scanner.h"
#include <stdlib.h>

void initTokenString (TokenString *token_buffer) {
    token_buffer->length = 0;
    token_buffer->capacity = 8;
    token_buffer->str = malloc(token_buffer->capacity * sizeof(char));

    if (token_buffer->str == NULL) {
        exit(99);
    }
}

void addChar (TokenString *token_buffer, char c) {
    if (token_buffer->length == token_buffer->capacity) {
        token_buffer->capacity = token_buffer->capacity * 2;
        token_buffer->str = realloc(token_buffer->str, token_buffer->capacity * sizeof(char));
        
        if (token_buffer->str == NULL) {
            exit(99);
        }
    }

    token_buffer->str[token_buffer->length] = c;
    token_buffer->length = token_buffer->length + 1;
}

void freeTokenString (TokenString *token_buffer) {
    free(token_buffer->str);
    token_buffer->capacity = 0;
    token_buffer->length = 0;
    token_buffer->str = NULL;
}