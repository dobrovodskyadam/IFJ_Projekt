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
 STATE_IS_COMMENT,
 STATE_COMMENT,
 STATE_COMMENT_NESTING,
 STATE_COMMENT_NESTING_START,
 STATE_COMMENT_NESTING_END,
 STATE_COMMENT_NESTING_AFTER,
 STATE_INDENT,
 STATE_CONCATENATION,
 STATE_DOT
 }STATE;

int initScannerData (ScannerData *scanner) {
    scanner->stack_capacity = 8;
    scanner->indent_stack = malloc(scanner->stack_capacity * sizeof(int));
    if (scanner->indent_stack == NULL) {
        return 0;
    }
    scanner->indent_stack[0] = 0;
    scanner->stack_top = 0;
    scanner->dedent = 0;
    scanner->new_line = 1;
    return 1;
}

int addStackCapacity (ScannerData *scanner) {
    int new_capacity = scanner->stack_capacity * 2;
    int *tmp = realloc(scanner->indent_stack, new_capacity * sizeof(int));
    if (tmp == NULL) {
        return 0;
    }
    scanner->indent_stack = tmp;
    scanner->stack_capacity = new_capacity;
    return 1;
}

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

void getToken (Token *token, ScannerData *scanner) {
    if (scanner->dedent > 0) {
        scanner->dedent--;
        token->type = TOKEN_DEDENT;
        return;
    }
    STATE state = STATE_START;
    TokenString token_buffer;
    if (!initTokenString(&token_buffer)) {
        token->type = TOKEN_ERROR_MEMORY;
        return;
    }
    int token_found = 0;
    int comment_nesting = 0;
    int comment_new_line = 0;
    int space_count = 0;
    int has_tab = 0;
    while (!token_found) {
        int c;
        c = fgetc(stdin);
        switch (state)
        {
            case STATE_START:
                if (scanner->new_line == 1) {
                    if (c == ' ') {
                        space_count++;
                        state = STATE_INDENT;
                    } else if (c == '\n') {
                        state = STATE_START;
                    } else if (c == '\t') {
                        state = STATE_INDENT;
                        has_tab = 1;
                    } else {
                        ungetc(c, stdin);
                        state = STATE_INDENT;
                    }
                } else if (isalpha(c)) {
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
                    state = STATE_DOT;
                } else if (c == '_') {
                    state = STATE_ID;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else if (c == '+') {
                    token->type = TOKEN_PLUS;
                    state = STATE_CONCATENATION;
                } else if (c == '-') {
                    token->type = TOKEN_MINUS;
                    token_found = 1;
                } else if (c == '*') {
                    token->type = TOKEN_MUL;
                    token_found = 1;
                } else if (c == '/') {
                    state = STATE_IS_COMMENT;
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
                } else if (c == ')') {
                    token->type = TOKEN_RPAREN;
                    token_found = 1;
                } else if (c == '"') {
                    state = STATE_STR_QUOTE1;
                } else if (c == '\n') {
                    token->type = TOKEN_EOL;
                    scanner->new_line = 1;
                    token_found = 1;
                } else if (c == EOF) {
                    token->type = TOKEN_EOF;
                    token_found = 1;
                } else if (isspace(c)) {
                    state = STATE_START;
                } else if (c == ';') {
                    token->type = TOKEN_SEPARATOR;
                    token_found = 1;
                } else if (c == ':') {
                    token->type = TOKEN_COLON;
                    token_found = 1;
                } else if (c == '@') {
                    state = STATE_WORD;
                    if (!addChar(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else {
                    token->type = TOKEN_ERROR;
                    token_found = 1;
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
                    if (!addChar(&token_buffer, '\0')) {
                        state = STATE_ERROR_MEMORY;
                    } else {
                        token->value.double_val = atof(token_buffer.str);
                        token_found = 1;
                    }
                } else {
                    if (!addChar(&token_buffer, '\0')) {
                        state = STATE_ERROR_MEMORY;
                    } else {
                         if (token->type == TOKEN_INT) {
                        token->value.int_val = atoi(token_buffer.str);
                        }
                        if (token->type == TOKEN_DOUBLE) {
                            token->value.double_val = atof(token_buffer.str);
                        }
                        ungetc(c, stdin);
                        token_found = 1;
                    }
                }
                break;

            case STATE_DOT:
                if (isdigit(c)) {
                    if (!addChar(&token_buffer, '.')) {
                        state = STATE_ERROR_MEMORY;
                    } else {
                        if (!addChar(&token_buffer, c)) {
                            state = STATE_ERROR_MEMORY;
                        } else {
                            token->type = TOKEN_DOUBLE;
                            state = STATE_NUM;
                        }
                    }
                } else {
                    ungetc(c, stdin);
                    token->type = TOKEN_DOT;
                    token_found = 1;
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
                    if (!addChar(&token_buffer, '\0')) {
                        state = STATE_ERROR_MEMORY;
                    } else {
                        token->value.double_val = atof(token_buffer.str);
                        token_found = 1;
                    }
                } else {
                    if (!addChar(&token_buffer, '\0')) {
                        state = STATE_ERROR_MEMORY;
                    } else {
                        token->value.double_val = atof(token_buffer.str);
                        ungetc(c, stdin);
                        token_found = 1;
                    }
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
                }
                break;

            case STATE_CONCATENATION:
                if (c == '+') {
                    token->type = TOKEN_CONCATENATION;
                } else {
                    ungetc(c, stdin);
                }
                token_found = 1;
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
                    if (!addChar(&token_buffer, '\0')) {
                        state = STATE_ERROR_MEMORY;
                    } else {
                        ungetc(c, stdin);
                        if (token_buffer.str[0] == '@') {
                            if (strcmp(token_buffer.str, "@main") == 0) {
                                token->type = TOKEN_MAIN;
                            } else {
                                token->type = TOKEN_ERROR;
                            }
                        } else {
                            if (strcmp(token_buffer.str, "def") == 0) {
                                token->type = TOKEN_DEF;
                            } else if (strcmp(token_buffer.str, "do") == 0) {
                                token->type = TOKEN_DO;
                            } else if (strcmp(token_buffer.str, "Double") == 0) {
                                token->type = TOKEN_KW_DOUBLE;
                            } else if (strcmp(token_buffer.str, "else") == 0) {
                                token->type = TOKEN_ELSE;
                            } else if (strcmp(token_buffer.str, "if") == 0) {
                                token->type = TOKEN_IF;
                            } else if (strcmp(token_buffer.str, "import") == 0) {
                                token->type = TOKEN_IMPORT;
                            } else if (strcmp(token_buffer.str, "Int") == 0) {
                                token->type = TOKEN_KW_INT;
                            } else if (strcmp(token_buffer.str, "return") == 0) {
                                token->type = TOKEN_RETURN;
                            } else if (strcmp(token_buffer.str, "String") == 0) {
                                token->type = TOKEN_KW_STRING;
                            } else if (strcmp(token_buffer.str, "then") == 0) {
                                token->type = TOKEN_THEN;
                            } else if (strcmp(token_buffer.str, "Unit") == 0) {
                                token->type = TOKEN_UNIT;
                            } else if (strcmp(token_buffer.str, "val") == 0) {
                                token->type = TOKEN_VAL;
                            } else if (strcmp(token_buffer.str, "var") == 0) {
                                token->type = TOKEN_VAR;
                            } else if (strcmp(token_buffer.str, "while") == 0) {
                                token->type = TOKEN_WHILE;
                            } else if (strcmp(token_buffer.str, "Boolean") == 0) {
                                token->type = TOKEN_BOOLEAN;
                            } else if (strcmp(token_buffer.str, "case") == 0) {
                                token->type = TOKEN_CASE;
                            } else if (strcmp(token_buffer.str, "for") == 0) {
                                token->type = TOKEN_FOR;
                            } else if (strcmp(token_buffer.str, "List") == 0) {
                                token->type = TOKEN_LIST;
                            } else if (strcmp(token_buffer.str, "until") == 0) {
                                token->type = TOKEN_UNTIL;
                            } else if (strcmp(token_buffer.str, "to") == 0) {
                                token->type = TOKEN_TO;
                            } else if (strcmp(token_buffer.str, "by") == 0) {
                                token->type = TOKEN_BY;
                            } else {
                                token->type = TOKEN_ID;
                                token->value.str_val = malloc(token_buffer.capacity * sizeof(char));
                                if (token->value.str_val == NULL) {
                                    token->type = TOKEN_ERROR_MEMORY;
                                } else {
                                    strcpy(token->value.str_val, token_buffer.str);
                                }
                            }
                        }
                        token_found = 1;
                    }
                }
                break;

            case STATE_ID:
                if (token_buffer.str[0] == '@') {
                    token->type = TOKEN_ERROR;
                    token_found = 1;
                } else {
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
                    } else if (c == '.' && token_buffer.length == 5 && (strncmp(token_buffer.str, "ifj26", 5)) == 0) {
                        int next_c = fgetc(stdin);
                        if (next_c == '*') {
                            token->type = TOKEN_IFJ;
                        } else {
                            token->type = TOKEN_ERROR;
                        }
                        token_found = 1;
                    } else {
                        if (!addChar(&token_buffer, '\0')) {
                            state = STATE_ERROR_MEMORY;
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
                        }
                    }
                }
                break;

            case STATE_ASSIGN:
                if (token->type == TOKEN_LS && c == '=') {
                    token->type = TOKEN_LS_EQ;
                    token_found = 1;
                } else if (token->type == TOKEN_GR && c == '=') {
                    token->type = TOKEN_GR_EQ;
                    token_found = 1;
                } else if (token->type == TOKEN_ASSIGN && c == '=') {
                    token->type = TOKEN_EQ;
                    token_found = 1;
                } else if (token->type == TOKEN_NOT && c == '=') {
                    token->type = TOKEN_NEQ;
                    token_found = 1;
                } else if (token->type == TOKEN_LS && c == '-') {
                    token->type = TOKEN_GEN_ARROW;
                    token_found = 1;
                } else {
                    ungetc(c, stdin);
                    token_found = 1;
                }
                break;

            case STATE_AND:
                if (c == '&') {
                    token->type = TOKEN_AND;
                    token_found = 1;
                } else {
                    token->type = TOKEN_ERROR;
                    token_found = 1;
                }
                break;

            case STATE_OR:
                if (c == '|') {
                    token->type = TOKEN_OR;
                    token_found = 1;
                } else {
                    token->type = TOKEN_ERROR;
                    token_found = 1;
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
                }
                break;

            case STATE_STR:
                if (c == '"') {
                    if (!addChar(&token_buffer, '\0')) {
                        state = STATE_ERROR_MEMORY;
                    } else {
                        token->type = TOKEN_STRING;
                        token->value.str_val = malloc(token_buffer.capacity * sizeof(char));
                        if (token->value.str_val == NULL) {
                            token->type = TOKEN_ERROR_MEMORY;
                        } else {
                            strcpy(token->value.str_val, token_buffer.str);
                        }
                        token_found = 1;
                    }
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
                }
                break;
            
            case STATE_STR_QUOTE2:
                if (c == '"') {
                    state = STATE_STR_QUOTE3;
                } else {
                    if (!addChar(&token_buffer, '\0')) {
                        state = STATE_ERROR_MEMORY;
                    } else {
                        token->type = TOKEN_STRING;
                        token->value.str_val = malloc(token_buffer.capacity * sizeof(char));
                        if (token->value.str_val == NULL) {
                            token->type = TOKEN_ERROR_MEMORY;
                        } else {
                            strcpy(token->value.str_val, token_buffer.str);
                        }
                        token_found = 1;
                    }
                    ungetc(c, stdin);
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
                }
                break;
            
            case STATE_STR_QUOTE3_END:
                if (c == '"') {
                    if (!addChar(&token_buffer, '\0')) {
                        state = STATE_ERROR_MEMORY;
                    } else {
                        token->type = TOKEN_STRING;
                        token->value.str_val = malloc(token_buffer.capacity * sizeof(char));
                        if (token->value.str_val == NULL) {
                            token->type = TOKEN_ERROR_MEMORY;
                        } else {
                            strcpy(token->value.str_val, token_buffer.str);
                        }
                        token_found = 1;
                    }
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
                }
                break;

            case STATE_IS_COMMENT:
                if (c == '/') {
                    state = STATE_COMMENT;
                } else if (c == '*') {
                    comment_nesting++;
                    state = STATE_COMMENT_NESTING;
                } else {
                    ungetc(c, stdin);
                    token->type = TOKEN_DIV;
                    token_found = 1;
                }
                break;

            case STATE_COMMENT:
                if (c == '\n') {
                    token->type = TOKEN_EOL;
                    scanner->new_line = 1;
                    token_found = 1;
                } else if (c == EOF) {
                    token->type = TOKEN_EOF;
                    token_found = 1;
                } else {
                    state = STATE_COMMENT;
                }
                break;

            case STATE_COMMENT_NESTING:
                if (c == '/') {
                    state = STATE_COMMENT_NESTING_START;
                } else if (c == '*') {
                    state = STATE_COMMENT_NESTING_END;
                } else if (c == EOF) {
                    token_found = 1;
                    token->type = TOKEN_ERROR;
                } else if (c == '\n') {
                    comment_new_line++;
                } else {
                    state = STATE_COMMENT_NESTING;
                }
                break;

            case STATE_COMMENT_NESTING_START:
                if (c == '*') {
                    comment_nesting++;
                    state = STATE_COMMENT_NESTING;
                } else if (c == EOF) {
                    token_found = 1;
                    token->type = TOKEN_ERROR;
                } else if (c == '\n') {
                    comment_new_line++;
                    state = STATE_COMMENT_NESTING;
                } else {
                    state = STATE_COMMENT_NESTING;
                }
                break;

            case STATE_COMMENT_NESTING_END:
                if (c == '/') {
                    comment_nesting--;
                    if (comment_nesting == 0){
                        if (comment_new_line == 0) {
                            state = STATE_START;
                        } else {
                            state = STATE_COMMENT_NESTING_AFTER;
                        }
                    } else {
                        state = STATE_COMMENT_NESTING;
                    }
                } else if (c == '*') {
                    state = STATE_COMMENT_NESTING_END;
                } else if (c == EOF) {
                    token_found = 1;
                    token->type = TOKEN_ERROR;
                } else if (c == '\n') {
                    comment_new_line++;
                    state = STATE_COMMENT_NESTING;
                } else {
                    state = STATE_COMMENT_NESTING;
                }
                break;

            case STATE_COMMENT_NESTING_AFTER:
                comment_new_line = 0;
                if (c == '\n') {
                    token->type = TOKEN_EOL;
                    scanner->new_line = 1;
                    token_found = 1;
                } else if (isspace(c)) {
                    state = STATE_COMMENT_NESTING_AFTER;
                } else if (c == '/') {
                    int next_c = fgetc(stdin);
                    if (next_c == '/') {
                        state = STATE_COMMENT;
                    } else if (next_c == '*') {
                        comment_nesting++;
                        state = STATE_COMMENT_NESTING;
                    } else {
                        token_found = 1;
                        token->type = TOKEN_ERROR;
                    }
                } else if (c == EOF) {
                    token_found = 1;
                    token->type = TOKEN_EOF;
                } else {
                    token_found = 1;
                    token->type = TOKEN_ERROR;
                }
                break;

            case STATE_INDENT:
                if (c == ' ') {
                    space_count++;
                    state = STATE_INDENT;
                } else if (c == '\t') {
                    has_tab = 1;
                    state = STATE_INDENT;
                } else if (c == '\n') {
                    has_tab = 0;
                    space_count = 0;
                    state = STATE_START;
                } else if ( c == '/') {
                    int next_c = fgetc(stdin);
                    if (next_c == '/') {
                        has_tab = 0;
                        space_count = 0;
                        state = STATE_COMMENT;
                    } else if (next_c == '*') {
                        has_tab = 0;
                        space_count = 0;
                        comment_nesting++;
                        state = STATE_COMMENT_NESTING;
                    } else {
                        token->type = TOKEN_ERROR;
                        token_found = 1;
                    }
                } else {
                    if (has_tab) {
                        token->type = TOKEN_ERROR;
                        token_found = 1;
                        break;
                    }
                    ungetc(c, stdin);
                    int last_space_count = scanner->indent_stack[scanner->stack_top];
                    if (space_count > last_space_count) {
                        scanner->stack_top++;
                        if (scanner->stack_top == scanner->stack_capacity) {
                            if (!addStackCapacity(scanner)) {
                                state = STATE_ERROR_MEMORY;
                                break;
                            }
                        }
                        scanner->indent_stack[scanner->stack_top] = space_count;
                        scanner->new_line = 0;
                        token->type = TOKEN_INDENT;
                        token_found = 1;
                    } else if (space_count < last_space_count) {
                        while (space_count < last_space_count && scanner->stack_top > 0) {
                            scanner->stack_top--;
                            last_space_count = scanner->indent_stack[scanner->stack_top];
                            scanner->dedent++;
                        }

                        if (space_count != last_space_count) {
                            token->type = TOKEN_ERROR;
                            token_found = 1;
                        } else {
                            token->type = TOKEN_DEDENT;
                            scanner->dedent--;
                            scanner->new_line = 0;
                            token_found = 1;
                        }
                    } else {
                        scanner->new_line = 0;
                        state = STATE_START;
                    }
                }
                break;

            default:
                break;
        }

        if (state == STATE_ERROR_MEMORY) {
            token->type = TOKEN_ERROR_MEMORY;
            token_found = 1;
        }

    }
    freeTokenString(&token_buffer);
}