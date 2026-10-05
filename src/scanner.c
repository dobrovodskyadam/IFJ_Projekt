#include "scanner.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

typedef enum 
{
 STATE_START,
 STATE_NUM,
 STATE_NUM_DOT,
 STATE_EXP,
 STATE_EXP_SIGN,
 STATE_EXP_NUM
 }STATE;

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

void getToken (Token *token) {
    STATE state = STATE_START;
    TokenString token_buffer;
    initTokenString(&token_buffer);
    while (1) {
        int c;
        c = fgetc(stdin);
        switch (state)
        {
            case STATE_START:
                if (isdigit(c)) {
                    state = STATE_NUM;
                    addChar(&token_buffer, c);
                    token->type = TOKEN_INT;
                } else if (c == '.') {
                    state = STATE_NUM_DOT;
                    addChar(&token_buffer, c);
                    token->type = TOKEN_DOUBLE;
                } else if (c == '+') {
                    token->type = TOKEN_PLUS;
                    return;
                } else if (c == '-') {
                    token->type = TOKEN_MINUS;
                    return;
                } else if (c == '*') {
                    token->type = TOKEN_MUL;
                    return;
                } else if (c == '/') {
                    token->type = TOKEN_DIV;
                    return;
                } else {
                    token->type = TOKEN_ERROR;
                    return;
                }
                break;

            case STATE_NUM:
                if (isdigit(c)) {
                    state = STATE_NUM;
                    addChar(&token_buffer, c);
                } else if (token->type == TOKEN_INT && c == '.') {
                    addChar(&token_buffer, c);
                    token->type = TOKEN_DOUBLE;
                    state = STATE_NUM_DOT;
                } else if(c == 'e' || c == 'E') {
                    addChar(&token_buffer, c);
                    token->type = TOKEN_DOUBLE;
                    state = STATE_EXP;
                } else if (token->type == TOKEN_DOUBLE && (c == 'd' || c == 'D')) {
                    token->value.double_val = atof(token_buffer.str);
                    return;
                } else {
                    if (token->type == TOKEN_INT) {
                        token->value.int_val = atoi(token_buffer.str);
                    }
                    if (token->type == TOKEN_DOUBLE) {
                        token->value.double_val = atof(token_buffer.str);
                    }
                    return;
                }
                break;

            case STATE_NUM_DOT:
                if (isdigit(c)) {
                    state = STATE_NUM;
                    addChar(&token_buffer, c);
                } else {
                    token->type = TOKEN_ERROR;
                    return;
                }
                break;
            
            case STATE_EXP:
                if (isdigit(c)) {
                    state = STATE_EXP_NUM;
                    addChar(&token_buffer, c);
                } else if (c == '+' || c == '-') {
                    state = STATE_EXP_SIGN;
                    addChar(&token_buffer, c);
                } else {
                    token->type = TOKEN_ERROR;
                    return;
                }
            break;

            case STATE_EXP_SIGN:
                if (isdigit(c)) {
                    state = STATE_EXP_NUM;
                    addChar(&token_buffer, c);
                } else {
                    token->type = TOKEN_ERROR;
                    return;
                }
            break;

            case STATE_EXP_NUM:
                if (isdigit(c)) {
                    state = STATE_EXP_NUM;
                    addChar(&token_buffer, c);
                } else if (c == 'd' || c == 'D'){
                    token->value.double_val = atof(token_buffer.str);
                    return;
                } else {
                    token->value.double_val = atof(token_buffer.str);
                    return;
                }
            break;
        }
    }
}
