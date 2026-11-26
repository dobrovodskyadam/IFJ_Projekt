/// @file scanner.c
/// @brief Module for getting tokens from stdin
/// @date 17-11-2026
///
/// @author Petr Rysavka (xrysavp00)
/// @author Email: xrysavp00@stud.fit.vut.cz
/// Faculty of Information Technology, Brno University of Technology

#include "scanner.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INDENT_STACK_INIT_CAPACITY 8
#define TOKEN_BUFFER_INIT_CAPACITY 8

/// @brief Enumeration of all scanner FSM states
typedef enum {
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
    STATE_DOT,
    STATE_PAREN_INDENT
} STATE;

/// @brief Initialization of structure to hold scanner information from function getToken
/// @param scanner Pointer to the scanner data structue to initialize
/// @return 1 if succesfull, otherwise 0
int init_scanner_data(ScannerData *scanner) {
    scanner->stack_capacity = INDENT_STACK_INIT_CAPACITY;
    scanner->indent_stack = malloc(scanner->stack_capacity * sizeof(int));
    if (scanner->indent_stack == NULL) {
        return 0;
    }
    scanner->indent_stack[0] = 0;
    scanner->stack_top = 0;
    scanner->dedent = 0;
    scanner->new_line = 1;
    scanner->paren_depth = 0;
    scanner->last_char = '\n';
    scanner->line = 1;
    return 1;
}

/// @brief Add more capacity for scanner, if capacity for indent stack is full
/// @param scanner Pointer to the scanner data structure
/// @return 1 if succesfull, otherwise 0
int add_stack_capacity(ScannerData *scanner) {
    int new_capacity = scanner->stack_capacity * 2;
    int *tmp = realloc(scanner->indent_stack, new_capacity * sizeof(int));
    if (tmp == NULL) {
        return 0;
    }
    scanner->indent_stack = tmp;
    scanner->stack_capacity = new_capacity;
    return 1;
}

/// @brief Initialization of the token string buffer for token
/// @param token_buffer Pointer to the token string buffer to initialize
/// @return 1 if succesfull, otherwise 0
int init_token_string(TokenString *token_buffer) {
    token_buffer->length = 0;
    token_buffer->capacity = TOKEN_BUFFER_INIT_CAPACITY;
    token_buffer->str = malloc(token_buffer->capacity * sizeof(char));

    if (token_buffer->str == NULL) {
        return 0;
    }
    return 1;
}

/// @brief Add new character to the token string buffer
/// @param token_buffer Pointer to the token string buffer
/// @param c Character to add
/// @return 1 if succesfull, otherwise 0
int add_char(TokenString *token_buffer, char c) {
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

int read_char(ScannerData *scanner) {
    int c = fgetc(stdin);
    if (c == EOF && scanner->last_char != '\n' && scanner->last_char != EOF) {
        c = '\n';
    }
    scanner->last_char = c;
    return c;
}

void unread_char(int c, FILE *source, ScannerData *scanner) {
    if (c == '\n') {
        scanner->line--;
    }
    ungetc(c, source);
}

void free_scanner_data(ScannerData *scanner) {
    free(scanner->indent_stack);
    scanner->indent_stack = NULL;
}

/// @brief Free allocated memory of the token string buffer
/// @param token_buffer Pointer to the token string buffer to free
void free_token_string(TokenString *token_buffer) {
    free(token_buffer->str);
    token_buffer->str = NULL;
}

/// @brief Main function. Gets next token from stdin
/// @param token Pointer to structure where is token store
/// @param scanner Pointer to the scanner data structure
void get_token(Token *token, ScannerData *scanner) {
    // if dedent is not done from previouse
    if (scanner->dedent > 0) {
        scanner->dedent--;
        token->type = TOKEN_DEDENT;
        token->line = scanner->line;
        return;
    }

    STATE state = STATE_START;
    TokenString token_buffer;
    if (!init_token_string(&token_buffer)) {
        token->type = TOKEN_ERROR_MEMORY;
        return;
    }

    int token_found = 0;
    int comment_nesting = 0;
    int comment_new_line = 0;
    int space_count = 0;
    int has_tab = 0;
    // implement of Finite State Machine
    while (!token_found) {
        int c;
        c = read_char(scanner);

        if (c == '\n') {
            scanner->line++;
        }

        switch (state) {
        case STATE_START:
            if (scanner->new_line == 1) {
                if (c == ' ') {
                    space_count++;
                    state = STATE_INDENT;
                } else if (c == '\n') {
                    space_count = 0;
                    has_tab = 0;
                    state = STATE_START;
                } else if (c == '\t') {
                    state = STATE_INDENT;
                    has_tab = 1;
                } else {
                     unread_char(c, stdin, scanner);
                    state = STATE_INDENT;
                }
            } else if (isalpha(c)) {
                state = STATE_WORD;
                if (!add_char(&token_buffer, c)) {
                    state = STATE_ERROR_MEMORY;
                }
            } else if (isdigit(c)) {
                state = STATE_NUM;
                if (!add_char(&token_buffer, c)) {
                    state = STATE_ERROR_MEMORY;
                } else {
                    token->type = TOKEN_INT;
                }
            } else if (c == '.') {
                state = STATE_DOT;
            } else if (c == '_') {
                state = STATE_ID;
                if (!add_char(&token_buffer, c)) {
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
            } else if (c == ',') {
              token->type = TOKEN_COMMA;
              token_found = 1;
              break;
            }
            else if (c == '[') {
              token->type = TOKEN_LBRACKET;
              token_found = 1;
              break;
            } else if (c == ']') {
              token->type = TOKEN_RBRACKET;
              token_found = 1;
               break;
            } else if (c == '(') {
                token->type = TOKEN_LPAREN;
                scanner->paren_depth++;
                token_found = 1;
            } else if (c == ')') {
                if (scanner->paren_depth > 0) {
                    scanner->paren_depth--;
                }
                token->type = TOKEN_RPAREN;
                token_found = 1;
            } else if (c == '"') {
                state = STATE_STR_QUOTE1;
            } else if (c == '\n') {
                if (scanner->paren_depth > 0) {
                    space_count = 0;
                    state = STATE_PAREN_INDENT;
                } else {
                    token->type = TOKEN_EOL;
                    scanner->new_line = 1;
                    token_found = 1;
                }
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
                if (!add_char(&token_buffer, c)) {
                    state = STATE_ERROR_MEMORY;
                }
            } else {
                token->type = TOKEN_ERROR;
                token_found = 1;
            }
            break;

        case STATE_NUM:
            if ((isdigit(c) || c == '_') && token_buffer.str[0] == '0' && token_buffer.length == 1 && token->type == TOKEN_INT) {
                token->type = TOKEN_ERROR;
                token_found = 1;
            }else if (isdigit(c)) {
                state = STATE_NUM;
                if (!add_char(&token_buffer, c)) {
                    state = STATE_ERROR_MEMORY;
                }
            } else if (token->type == TOKEN_INT && c == '.') {
                state = STATE_NUM_DOT;
                if (!add_char(&token_buffer, c)) {
                    state = STATE_ERROR_MEMORY;
                } else {
                    token->type = TOKEN_DOUBLE;
                }
            } else if (c == 'e' || c == 'E') {
                state = STATE_EXP;
                if (!add_char(&token_buffer, c)) {
                    state = STATE_ERROR_MEMORY;
                } else {
                    token->type = TOKEN_DOUBLE;
                }
            } else if (c == '_') {
                state = STATE_NUM_UNDERSCORE;
            } else if (token->type == TOKEN_DOUBLE && (c == 'd' || c == 'D')) {
                if (!add_char(&token_buffer, '\0')) {
                    state = STATE_ERROR_MEMORY;
                } else {
                    token->value.double_val = atof(token_buffer.str);
                    token_found = 1;
                }
            } else {
                if (!add_char(&token_buffer, '\0')) {
                    state = STATE_ERROR_MEMORY;
                } else {
                    if (token->type == TOKEN_INT) {
                        token->value.int_val = atoi(token_buffer.str);
                    }
                    if (token->type == TOKEN_DOUBLE) {
                        token->value.double_val = atof(token_buffer.str);
                    }
                    unread_char(c, stdin, scanner);
                    token_found = 1;
                }
            }
            break;

        case STATE_DOT:
            if (isdigit(c)) {
                if (!add_char(&token_buffer, '.')) {
                    state = STATE_ERROR_MEMORY;
                } else {
                    if (!add_char(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    } else {
                        token->type = TOKEN_DOUBLE;
                        state = STATE_NUM;
                    }
                }
            } else {
                unread_char(c, stdin, scanner);
                token->type = TOKEN_DOT;
                token_found = 1;
            }
            break;

        case STATE_NUM_DOT:
            if (isdigit(c)) {
                state = STATE_NUM;
                if (!add_char(&token_buffer, c)) {
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
                if (!add_char(&token_buffer, c)) {
                    state = STATE_ERROR_MEMORY;
                }
            } else if (c == '_') {
                state = STATE_NUM_UNDERSCORE;
            } else {
                token->type = TOKEN_ERROR;
                token_found = 1;
            }
            break;

        case STATE_EXP:
            if (isdigit(c)) {
                state = STATE_EXP_NUM;
                if (!add_char(&token_buffer, c)) {
                    state = STATE_ERROR_MEMORY;
                }
            } else if (c == '+' || c == '-') {
                state = STATE_EXP_SIGN;
                if (!add_char(&token_buffer, c)) {
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
                if (!add_char(&token_buffer, c)) {
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
                if (!add_char(&token_buffer, c)) {
                    state = STATE_ERROR_MEMORY;
                }
            } else if (c == '_') {
                state = STATE_EXP_UNDERSCORE;
            } else if (c == 'd' || c == 'D') {
                if (!add_char(&token_buffer, '\0')) {
                    state = STATE_ERROR_MEMORY;
                } else {
                    token->value.double_val = atof(token_buffer.str);
                    token_found = 1;
                }
            } else {
                if (!add_char(&token_buffer, '\0')) {
                    state = STATE_ERROR_MEMORY;
                } else {
                    token->value.double_val = atof(token_buffer.str);
                    unread_char(c, stdin, scanner);
                    token_found = 1;
                }
            }
            break;

        case STATE_EXP_UNDERSCORE:
            if (isdigit(c)) {
                state = STATE_EXP_NUM;
                if (!add_char(&token_buffer, c)) {
                    state = STATE_ERROR_MEMORY;
                }
            } else if (c == '_') {
                state = STATE_EXP_UNDERSCORE;
            } else {
                token->type = TOKEN_ERROR;
                token_found = 1;
            }
            break;

        case STATE_CONCATENATION:
            if (c == '+') {
                token->type = TOKEN_CONCATENATION;
            } else {
                unread_char(c, stdin, scanner);
            }
            token_found = 1;
            break;

        case STATE_WORD:
            if (isalpha(c)) {
                state = STATE_WORD;
                if (!add_char(&token_buffer, c)) {
                    state = STATE_ERROR_MEMORY;
                }
            } else if (isdigit(c)) {
                state = STATE_ID;
                if (!add_char(&token_buffer, c)) {
                    state = STATE_ERROR_MEMORY;
                }
            } else if (c == '_') {
                state = STATE_ID;
                if (!add_char(&token_buffer, c)) {
                    state = STATE_ERROR_MEMORY;
                }
            } else {
                if (!add_char(&token_buffer, '\0')) {
                    state = STATE_ERROR_MEMORY;
                } else {
                    unread_char(c, stdin, scanner);
                    // cheking if its @main
                    if (token_buffer.str[0] == '@') {
                        if (strcmp(token_buffer.str, "@main") == 0) {
                            token->type = TOKEN_MAIN;
                        } else {
                            token->type = TOKEN_ERROR;
                        }
                    } else {
                        // keywords cheking
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
                        } else if (strcmp(token_buffer.str, "true") == 0) {
                           token->type = TOKEN_TRUE;
                        } else if (strcmp(token_buffer.str, "false") == 0) {
                          token->type = TOKEN_FALSE;
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
                    if (!add_char(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else if (isdigit(c)) {
                    state = STATE_ID;
                    if (!add_char(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                } else if (c == '_') {
                    state = STATE_ID;
                    if (!add_char(&token_buffer, c)) {
                        state = STATE_ERROR_MEMORY;
                    }
                    // checking if token_buffer length is 5 and with strncmp if its ifj26 (strncmp
                    // doesnt need \0)
                } else if (c == '.' && token_buffer.length == 5 &&
                           (strncmp(token_buffer.str, "ifj26", 5)) == 0) {
                    int next_c = fgetc(stdin);
                    if (next_c == '*') {
                        token->type = TOKEN_IFJ;
                    } else {
                        token->type = TOKEN_ERROR;
                    }
                    token_found = 1;
                } else {
                    if (!add_char(&token_buffer, '\0')) {
                        state = STATE_ERROR_MEMORY;
                    } else if (strcmp(token_buffer.str, "_") == 0) {
                        token->type = TOKEN_ERROR;
                        token_found = 1;
                    } else {
                        unread_char(c, stdin, scanner);
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
                unread_char(c, stdin, scanner);
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
            } else if (isascii(c) && c != '\n') {
                state = STATE_STR;
                if (!add_char(&token_buffer, c)) {
                    state = STATE_ERROR_MEMORY;
                }
            } else {
                token->type = TOKEN_ERROR;
                token_found = 1;
            }
            break;

        case STATE_STR:
            if (c == '"') {
                if (!add_char(&token_buffer, '\0')) {
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
            } else if (isascii(c) && c != '\n') {
                state = STATE_STR;
                if (!add_char(&token_buffer, c)) {
                    state = STATE_ERROR_MEMORY;
                }
            } else {
                token->type = TOKEN_ERROR;
                token_found = 1;
            }
            break;

        case STATE_ESCAPE:
            if (c == '\\') {
                state = STATE_STR;
                if (!add_char(&token_buffer, '\\')) {
                    state = STATE_ERROR_MEMORY;
                }
            } else if (c == '"') {
                state = STATE_STR;
                if (!add_char(&token_buffer, '\"')) {
                    state = STATE_ERROR_MEMORY;
                }
            } else if (c == 'n') {
                state = STATE_STR;
                if (!add_char(&token_buffer, '\n')) {
                    state = STATE_ERROR_MEMORY;
                }
            } else if (c == 'r') {
                state = STATE_STR;
                if (!add_char(&token_buffer, '\r')) {
                    state = STATE_ERROR_MEMORY;
                }
            } else if (c == 't') {
                state = STATE_STR;
                if (!add_char(&token_buffer, '\t')) {
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
                // end of one-line string
            } else {
                if (!add_char(&token_buffer, '\0')) {
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
                unread_char(c, stdin, scanner);
            }
            break;

        case STATE_STR_QUOTE3:
            if (c == '"') {
                state = STATE_STR_QUOTE2_END;
            } else if (isascii(c)) {
                state = STATE_STR_QUOTE3;
                if (!add_char(&token_buffer, c)) {
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
                if (!add_char(&token_buffer, '"')) {
                    state = STATE_ERROR_MEMORY;
                }
                if (!add_char(&token_buffer, c)) {
                    state = STATE_ERROR_MEMORY;
                }
            } else {
                token->type = TOKEN_ERROR;
                token_found = 1;
            }
            break;

        case STATE_STR_QUOTE3_END:
            // end of multi-line string
            if (c == '"') {
                if (!add_char(&token_buffer, '\0')) {
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
                // string continue
            } else if (isascii(c)) {
                state = STATE_STR_QUOTE3;
                if (!add_char(&token_buffer, '"')) {
                    state = STATE_ERROR_MEMORY;
                }
                if (!add_char(&token_buffer, '"')) {
                    state = STATE_ERROR_MEMORY;
                }
                if (!add_char(&token_buffer, c)) {
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
                unread_char(c, stdin, scanner);
                token->type = TOKEN_DIV;
                token_found = 1;
            }
            break;

        case STATE_COMMENT:
            if (c == '\n') {
                if (scanner->paren_depth > 0) {
                    space_count = 0;
                    has_tab = 0;
                    state = STATE_PAREN_INDENT;
                } else if (scanner->new_line == 1) {
                    space_count = 0;
                    has_tab = 0;
                    state = STATE_START;
                } else {
                    token->type = TOKEN_EOL;
                    scanner->new_line = 1;
                    token_found = 1;
                }
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
            } else if (c == '/') {
                state = STATE_COMMENT_NESTING_START;
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
                if (comment_nesting == 0) {
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
                if (scanner->paren_depth > 0) {
                    space_count = 0;
                    has_tab = 0;
                    state = STATE_PAREN_INDENT;
                } else if (scanner->new_line == 1) {
                    space_count = 0;
                    has_tab = 0;
                    state = STATE_START;
                } else {
                    token->type = TOKEN_EOL;
                    scanner->new_line = 1;
                    token_found = 1;
                }
            } else if (isspace(c)) {
                state = STATE_COMMENT_NESTING_AFTER;
                // checking if there is another comment
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
                // checkin if it comments
            } else if (c == '/') {
                int next_c = fgetc(stdin);
                if (next_c == '/') {
                    state = STATE_COMMENT;
                } else if (next_c == '*') {
                    comment_nesting++;
                    state = STATE_COMMENT_NESTING;
                } else {
                    ungetc(next_c, stdin);
                    if (has_tab) {
                        token->type = TOKEN_ERROR;
                    } else {
                        token->type = TOKEN_DIV;
                        scanner->new_line = 0;
                    }
                    token_found = 1;
                }
            } else {
                if (has_tab) {
                    token->type = TOKEN_ERROR;
                    token_found = 1;
                    break;
                }
                unread_char(c, stdin, scanner);
                int last_space_count = scanner->indent_stack[scanner->stack_top];
                // indent is bigger than last indent
                if (space_count > last_space_count) {
                    scanner->stack_top++;
                    if (scanner->stack_top == scanner->stack_capacity) {
                        if (!add_stack_capacity(scanner)) {
                            state = STATE_ERROR_MEMORY;
                            break;
                        }
                    }
                    scanner->indent_stack[scanner->stack_top] = space_count;
                    scanner->new_line = 0;
                    token->type = TOKEN_INDENT;
                    token_found = 1;
                    // indent is smaller than last indent
                } else if (space_count < last_space_count) {
                    while (space_count < last_space_count && scanner->stack_top > 0) {
                        scanner->stack_top--;
                        last_space_count = scanner->indent_stack[scanner->stack_top];
                        scanner->dedent++;
                    }

                    // indent is not same with any in indent_stack
                    if (space_count != last_space_count) {
                        token->type = TOKEN_ERROR;
                        token_found = 1;
                    } else {
                        token->type = TOKEN_DEDENT;
                        scanner->dedent--;
                        scanner->new_line = 0;
                        token_found = 1;
                    }
                    // indent is same as last indent
                } else {
                    scanner->new_line = 0;
                    state = STATE_START;
                }
            }
            break;

        case STATE_PAREN_INDENT:
            if (c == ' ') {
                space_count++;
                state = STATE_PAREN_INDENT;
            } else if (c == '\t') {
                has_tab = 1;
                state = STATE_PAREN_INDENT;
            } else if (c == '\n') {
                has_tab = 0;
                space_count = 0;
                state = STATE_PAREN_INDENT;
                // checkin if it comments
            } else if (c == '/') {
                int next_c = fgetc(stdin);
                if (next_c == '/') {
                    state = STATE_COMMENT;
                } else if (next_c == '*') {
                    comment_nesting++;
                    state = STATE_COMMENT_NESTING;
                } else {
                    ungetc(next_c, stdin);
                    if (has_tab || space_count <= scanner->indent_stack[scanner->stack_top]) {
                        token->type = TOKEN_ERROR;
                    } else {
                        token->type = TOKEN_DIV;
                    }
                    token_found = 1;
                }
            } else {
                if (c != ')' && has_tab) {
                    token->type = TOKEN_ERROR;
                    token_found = 1;
                } else if (c != ')' && space_count <= scanner->indent_stack[scanner->stack_top]) {
                    scanner->paren_depth = 0;
                    unread_char(c, stdin, scanner);
                    scanner->new_line = 1;
                    token->type = TOKEN_EOL;
                    token_found = 1;
                } else {
                    unread_char(c, stdin, scanner);
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

    token->line = (token->type == TOKEN_EOL) ? scanner->line - 1 : scanner->line;
    free_token_string(&token_buffer);
}


// Helper Static Array
static const char *token_names[TOKEN_COUNT] = {
    [TOKEN_ID] = "TOKEN_ID",
    [TOKEN_INT] = "TOKEN_INT",
    [TOKEN_DOUBLE] = "TOKEN_DOUBLE",
    [TOKEN_STRING] = "TOKEN_STRING",
    [TOKEN_PLUS] = "TOKEN_PLUS",
    [TOKEN_MINUS] = "TOKEN_MINUS",
    [TOKEN_IF] = "TOKEN_IF",
    [TOKEN_ELSE] = "TOKEN_ELSE",
    [TOKEN_WHILE] = "TOKEN_WHILE",
    [TOKEN_FOR] = "TOKEN_FOR",
    [TOKEN_AND] = "TOKEN_AND",
    [TOKEN_OR] = "TOKEN_OR",
    [TOKEN_EQ] = "TOKEN_EQ",
    [TOKEN_GR_EQ] = "TOKEN_GR_EQ",
    [TOKEN_LS_EQ] = "TOKEN_LS_EQ",
    [TOKEN_LS] = "TOKEN_LS",
    [TOKEN_GR] = "TOKEN_GR",
    [TOKEN_DEF] = "TOKEN_DEF",
    [TOKEN_MUL] = "TOKEN_MUL",
    [TOKEN_DIV] = "TOKEN_DIV",
    [TOKEN_NEQ] = "TOKEN_NEQ",
    [TOKEN_LPAREN] = "TOKEN_LPAREN",
    [TOKEN_RPAREN] = "TOKEN_RPAREN",
    [TOKEN_ASSIGN] = "TOKEN_ASSIGN",
    [TOKEN_NOT] = "TOKEN_NOT",
    [TOKEN_ERROR] = "TOKEN_ERROR",
    [TOKEN_ERROR_MEMORY] = "TOKEN_ERROR_MEMORY",
    [TOKEN_VAR] = "TOKEN_VAR",
    [TOKEN_VAL] = "TOKEN_VAL",
    [TOKEN_RETURN] = "TOKEN_RETURN",
    [TOKEN_UNIT] = "TOKEN_UNIT",
    [TOKEN_BOOLEAN] = "TOKEN_BOOLEAN",
    [TOKEN_CASE] = "TOKEN_CASE",
    [TOKEN_LIST] = "TOKEN_LIST",
    [TOKEN_DO] = "TOKEN_DO",
    [TOKEN_THEN] = "TOKEN_THEN",
    [TOKEN_IMPORT] = "TOKEN_IMPORT",
    [TOKEN_KW_STRING] = "TOKEN_KW_STRING",
    [TOKEN_KW_INT] = "TOKEN_KW_INT",
    [TOKEN_KW_DOUBLE] = "TOKEN_KW_DOUBLE",
    [TOKEN_EOF] = "TOKEN_EOF",
    [TOKEN_INDENT] = "TOKEN_INDENT",
    [TOKEN_DEDENT] = "TOKEN_DEDENT",
    [TOKEN_EOL] = "TOKEN_EOL",
    [TOKEN_SEPARATOR] = "TOKEN_SEPARATOR",
    [TOKEN_COLON] = "TOKEN_COLON",
    [TOKEN_IFJ] = "TOKEN_IFJ",
    [TOKEN_MAIN] = "TOKEN_MAIN",
    [TOKEN_COMMA] = "TOKEN_COMMA",
    [TOKEN_TRUE] = "TOKEN_TRUE",
    [TOKEN_FALSE] = "TOKEN_FALSE",
    [TOKEN_LBRACKET] = "TOKEN_LBRACKET",
    [TOKEN_RBRACKET] = "TOKEN_RBRACKET",
    [TOKEN_DOT] = "TOKEN_DOT",
    [TOKEN_GEN_ARROW] = "TOKEN_GEN_ARROW",
    [TOKEN_CONCATENATION] = "TOKEN_CONCATENATION",
    [TOKEN_UNTIL] = "TOKEN_UNTIL",
    [TOKEN_TO] = "TOKEN_TO",
    [TOKEN_BY] = "TOKEN_BY",
};

// Function for getting the TokenName from the TokenType
const char *tokenTypeName(TokenType type) {
    if (type < 0 || type >= TOKEN_COUNT) {
        return "UNKNOWN";
    }
    return token_names[type];
}
