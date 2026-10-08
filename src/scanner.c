#include "scanner.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

typedef enum 
{
 STATE_START,
 STATE_NUM,
 STATE_NUM_DOT,
 STATE_NUM_UNDERSCORE,
 STATE_EXP,
 STATE_EXP_SIGN,
 STATE_EXP_NUM,
 STATE_EXP_UNDERSCORE,
 STATE_ASSIGN,
 STATE_AND,
 STATE_OR,
 STATE_STR,
 STATE_ESCAPE,
 STATE_STR_QUOTE1,
 STATE_STR_QUOTE2,
 STATE_STR_QUOTE3,
 STATE_STR_QUOTE2_END,
 STATE_STR_QUOTE3_END,
 STATE_ERROR_MEMORY,
 STATE_WORD,
 STATE_ID,
 STATE_IS_KW
 }STATE;

int initTokenString (TokenString *token_buffer) {
    token_buffer->length = 0;
    token_buffer->capacity = 8;
    token_buffer->str = malloc(token_buffer->capacity * sizeof(char));

    if (token_buffer->str == NULL) {
        return 0;
    }
    return 1;
}

int addChar (TokenString *token_buffer, char c) {
    if (token_buffer->length == token_buffer->capacity) {
        int new_capacity = token_buffer->capacity * 2;
        char *tmp = realloc(token_buffer->str, new_capacity * sizeof(char));

        if (tmp == NULL) {
            return 0;
        }
        token_buffer->str = tmp;
        token_buffer->capacity = new_capacity;
    }
    token_buffer->str[token_buffer->length] = c;
    token_buffer->length = token_buffer->length + 1;
    return 1;
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
    if (!initTokenString(&token_buffer)) {
        token->type = TOKEN_ERROR_MEMORY;
        return;
    }
    int token_found = 0;
    while (!token_found) {
        int c;
        c = fgetc(stdin);
        switch (state)
        {
            case STATE_START:
                if (isalpha(c)) {
                    state = STATE_WORD;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else if (isdigit(c)) {
                    state = STATE_NUM;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    } else {
                        token->type = TOKEN_INT;
                    }
                } else if (c == '.') {
                    state = STATE_NUM_DOT;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    } else {
                        token->type = TOKEN_DOUBLE;
                    }
                } else if (c == '_') {
                    state = STATE_ID;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else if (c == '+') {
                    token->type = TOKEN_PLUS;
                    token_found = 1;
                    break;
                } else if (c == '-') {
                    token->type = TOKEN_MINUS;
                    token_found = 1;
                    break;
                } else if (c == '*') {
                    token->type = TOKEN_MUL;
                    token_found = 1;
                    break;
                } else if (c == '/') {
                    token->type = TOKEN_DIV;
                    token_found = 1;
                    break;
                } else if (c == '<') {
                    state = STATE_ASSIGN;
                    token->type = TOKEN_LS;
                } else if (c == '>') {
                    state = STATE_ASSIGN;
                    token->type = TOKEN_GR;
                } else if (c == '=') {
                    state = STATE_ASSIGN;
                    token->type = TOKEN_ASSIGN;
                } else if (c == '!') {
                    state = STATE_ASSIGN;
                    token->type = TOKEN_NOT;
                } else if (c == '&') {
                    state = STATE_AND;
                } else if (c == '|') {
                    state = STATE_OR;
                } else if (c == '(') {
                    token->type = TOKEN_LPAREN;
                    token_found = 1;
                    break;
                } else if (c == ')') {
                    token->type = TOKEN_RPAREN;
                    token_found = 1;
                    break;
                } else if (c == '"') {
                    state = STATE_STR_QUOTE1;
                } else {
                    token->type = TOKEN_ERROR;
                    token_found = 1;
                    break;
                }
                break;

            case STATE_NUM:
                if (isdigit(c)) {
                    state = STATE_NUM;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else if (token->type == TOKEN_INT && c == '.') {
                    state = STATE_NUM_DOT;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    } else {
                        token->type = TOKEN_DOUBLE;
                    }
                } else if(c == 'e' || c == 'E') {
                    state = STATE_EXP;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    } else {
                        token->type = TOKEN_DOUBLE;
                    }
                } else if (c == '_') {
                    state = STATE_NUM_UNDERSCORE;
                } else if (token->type == TOKEN_DOUBLE && (c == 'd' || c == 'D')) {
                    token->value.double_val = atof(token_buffer.str);
                    token_found = 1;
                    break;
                } else {
                    if (token->type == TOKEN_INT) {
                        token->value.int_val = atoi(token_buffer.str);
                    }
                    if (token->type == TOKEN_DOUBLE) {
                        token->value.double_val = atof(token_buffer.str);
                    }
                    ungetc(c, stdin);
                    token_found = 1;
                    break;
                }
                break;

            case STATE_NUM_DOT:
                if (isdigit(c)) {
                    state = STATE_NUM;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else {
                    token->type = TOKEN_ERROR;
                    token_found = 1;
                    break;
                }
                break;

            case STATE_NUM_UNDERSCORE:
                if (isdigit(c)) {
                    state = STATE_NUM;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else {
                    token->type = TOKEN_ERROR;
                    token_found = 1;
                    break;
                }
                break;
            
            case STATE_EXP:
                if (isdigit(c)) {
                    state = STATE_EXP_NUM;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else if (c == '+' || c == '-') {
                    state = STATE_EXP_SIGN;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else {
                    token->type = TOKEN_ERROR;
                    token_found = 1;
                    break;
                }
                break;

            case STATE_EXP_SIGN:
                if (isdigit(c)) {
                    state = STATE_EXP_NUM;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else {
                    token->type = TOKEN_ERROR;
                    token_found = 1;
                    break;
                }
                break;

            case STATE_EXP_NUM:
                if (isdigit(c)) {
                    state = STATE_EXP_NUM;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else if (c == '_') {
                    state = STATE_EXP_UNDERSCORE;
                } else if (c == 'd' || c == 'D'){
                    token->value.double_val = atof(token_buffer.str);
                    token_found = 1;
                    break;
                } else {
                    token->value.double_val = atof(token_buffer.str);
                    ungetc(c, stdin);
                    token_found = 1;
                    break;
                }
                break;
            
            case STATE_EXP_UNDERSCORE:
                if (isdigit(c)) {
                    state = STATE_EXP_NUM;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else {
                    token->type = TOKEN_ERROR;
                    token_found = 1;
                    break;
                }
                break;

            case STATE_WORD:
                if (isalpha(c)) {
                    state = STATE_WORD;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else if (isdigit(c)) {
                    state = STATE_ID;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else if (c == '_') {
                    state = STATE_ID;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else {
                    ungetc(c, stdin);
                    state = STATE_IS_KW;
                }
                break;

            case STATE_ID: {
                if (isalpha(c)) {
                    state = STATE_ID;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else if (isdigit(c)) {
                    state = STATE_ID;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else if (c == '_') {
                    state = STATE_ID;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else {
                    ungetc(c, stdin);
                    token->type = TOKEN_ID;
                    token->value.str_val = malloc(token_buffer.capacity * sizeof(char));
                    if (token->value.str_val == NULL) {
                        token->type = TOKEN_ERROR_MEMORY;
                    } else {
                        strcpy(token->value.str_val, token_buffer.str);
                    }
                    token_found = 1;
                    break;
                }
                break;
            }

            case STATE_ASSIGN:
                if (token->type == TOKEN_LS && c == '=') {
                    token->type = TOKEN_LS_EQ;
                    token_found = 1;
                    break;
                } else if (token->type == TOKEN_GR && c == '=') {
                    token->type = TOKEN_GR_EQ;
                    token_found = 1;
                    break;
                } else if (token->type == TOKEN_ASSIGN && c == '=') {
                    token->type = TOKEN_EQ;
                    token_found = 1;
                    break;
                } else if (token->type == TOKEN_NOT && c == '=') {
                    token->type = TOKEN_NEQ;
                    token_found = 1;
                    break;
                } else {
                    ungetc(c, stdin);
                    token_found = 1;
                    break;
                }
                break;

            case STATE_AND:
                if (c == '&') {
                    token->type = TOKEN_AND;
                    token_found = 1;
                    break;
                } else {
                    token->type = TOKEN_ERROR;
                    token_found = 1;
                    break;
                }
                break;

            case STATE_OR:
                if (c == '|') {
                    token->type = TOKEN_OR;
                    token_found = 1;
                    break;
                } else {
                    token->type = TOKEN_ERROR;
                    token_found = 1;
                    break;
                }
                break;

            case STATE_STR_QUOTE1:
                if (c == '"') {
                    state = STATE_STR_QUOTE2;
                } else if (c == '\\') {
                    state = STATE_ESCAPE;
                } 
                else if (isascii(c) && c != '\n') {
                    state = STATE_STR;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else {
                    token->type = TOKEN_ERROR;
                    token_found = 1;
                    break;
                }
                break;

            case STATE_STR:
                if (c == '"') {
                    state = STATE_STR_QUOTE2;
                } else if (c == '\\') {
                    state = STATE_ESCAPE;
                } 
                else if (isascii(c) && c != '\n') {
                    state = STATE_STR;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else {
                    token->type = TOKEN_ERROR;
                    token_found = 1;
                    break;
                }
                break;

            case STATE_ESCAPE:
                if (c == '\\'){
                    state = STATE_STR;
                    if (!addChar(&token_buffer, '\\')) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else if (c == '"') {
                    state = STATE_STR;
                    if (!addChar(&token_buffer, '\"')) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else if (c == 'n') {
                    state = STATE_STR;
                    if (!addChar(&token_buffer, '\n')) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else if (c == 'r') {
                    state = STATE_STR;
                    if (!addChar(&token_buffer, '\r')) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else if (c == 't') {
                    state = STATE_STR;
                    if (!addChar(&token_buffer, '\t')) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else {
                    token->type = TOKEN_ERROR;
                    token_found = 1;
                    break;
                }
                break;
            
            case STATE_STR_QUOTE2:
                if (c == '"') {
                    state = STATE_STR_QUOTE3;
                } else {
                    token->type = TOKEN_STRING;
                    token->value.str_val = malloc(token_buffer.capacity * sizeof(char));
                    if (token->value.str_val == NULL) {
                        token->type = TOKEN_ERROR_MEMORY;
                    } else {
                        strcpy(token->value.str_val, token_buffer.str);
                    }
                    token_found = 1;
                    break;
                }
                break;

            case STATE_STR_QUOTE3:
                if (c == '"') {
                    state = STATE_STR_QUOTE2_END;
                } else if (isascii(c)) {
                    state = STATE_STR_QUOTE3;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else {
                    token->type = TOKEN_ERROR;
                    token_found = 1;
                    break;
                }
                break;

            case STATE_STR_QUOTE2_END:
                if (c == '"') {
                    state = STATE_STR_QUOTE3_END;
                } else if (isascii(c)) {
                    state = STATE_STR_QUOTE3;
                    if (!addChar(&token_buffer, '"')) {
                        state = STATE_ERROR_MEMORY;
                    }
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else {
                    token->type = TOKEN_ERROR;
                    token_found = 1;
                    break;
                }
                break;
            
            case STATE_STR_QUOTE3_END:
                if (c == '"') {
                    token->type = TOKEN_STRING;
                    token->value.str_val = malloc(token_buffer.capacity * sizeof(char));
                    if (token->value.str_val == NULL) {
                        token->type = TOKEN_ERROR_MEMORY;
                    } else {
                        strcpy(token->value.str_val, token_buffer.str);
                    }
                    token_found = 1;
                    break;
                } else if (isascii(c)) {
                    state = STATE_STR_QUOTE3;
                    if (!addChar(&token_buffer, '"')) {
                        state = STATE_ERROR_MEMORY;
                    }
                    if (!addChar(&token_buffer, '"')) {
                        state = STATE_ERROR_MEMORY;
                    }
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else {
                    token->type = TOKEN_ERROR;
                    token_found = 1;
                    break;
                }
                break;

            default:
                break;
        }

        if (state == STATE_ERROR_MEMORY) {
            token->type = TOKEN_ERROR_MEMORY;
            token_found = 1;
        }

        if (state == STATE_IS_KW) {
            token->type = TOKEN_ID;
            token->value.str_val = malloc(token_buffer.capacity * sizeof(char));
            if (token->value.str_val == NULL) {
                token->type = TOKEN_ERROR_MEMORY;
            } else {
                strcpy(token->value.str_val, token_buffer.str);
            }
            token_found = 1;
        }
    }
    freeTokenString(&token_buffer);
}