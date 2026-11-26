/// @file scanner.h
/// @brief Header file for module scanner.c
/// @date 17-11-2026
///
/// @author Petr Rysavka (xrysavp00)
/// @author Email: xrysavp00@stud.fit.vut.cz
/// Faculty of Information Technology, Brno University of Technology
#ifndef SCANNER_H
#define SCANNER_H

/// @brief Enumeration of all tokens types
typedef enum {
    TOKEN_ID,
    TOKEN_INT,
    TOKEN_DOUBLE,
    TOKEN_STRING,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_EQ,
    TOKEN_GR_EQ,
    TOKEN_LS_EQ,
    TOKEN_LS,
    TOKEN_GR,
    TOKEN_DEF,
    TOKEN_MUL,
    TOKEN_DIV,
    TOKEN_NEQ,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_ASSIGN,
    TOKEN_NOT,
    TOKEN_ERROR,
    TOKEN_ERROR_MEMORY,
    TOKEN_VAR,
    TOKEN_VAL,
    TOKEN_RETURN,
    TOKEN_UNIT,
    TOKEN_BOOLEAN,
    TOKEN_CASE,
    TOKEN_LIST,
    TOKEN_DO,
    TOKEN_THEN,
    TOKEN_IMPORT,
    TOKEN_KW_STRING,
    TOKEN_KW_INT,
    TOKEN_KW_DOUBLE,
    TOKEN_EOF,
    TOKEN_INDENT,
    TOKEN_DEDENT,
    TOKEN_EOL,
    TOKEN_SEPARATOR,
    TOKEN_COLON,
    TOKEN_IFJ,
    TOKEN_MAIN,
    TOKEN_DOT,
    TOKEN_GEN_ARROW,
    TOKEN_CONCATENATION,
    TOKEN_UNTIL,
    TOKEN_TO,
    TOKEN_BY,
    TOKEN_COMMA,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_COUNT
} TokenType;

/// @brief Structure representing token
typedef struct {
    TokenType type;
    unsigned int line;
    union {
        int int_val;
        double double_val;
        char *str_val;
    } value;
} Token;

/// @brief Structure of token string buffer
typedef struct {
    char *str;
    int length;
    int capacity;
} TokenString;

/// @brief Structure to hold scanner information from function getToken
typedef struct {
    int stack_capacity;
    int *indent_stack;
    int stack_top;
    int dedent;
    int new_line;
    int paren_depth;
    int last_char;
    unsigned line;
} ScannerData;


#ifdef __cplusplus
extern "C" {
#endif

int init_scanner_data(ScannerData *scanner);
void get_token(Token *token, ScannerData *scanner);
const char *tokenTypeName(TokenType type);

#ifdef __cplusplus
}
#endif

#endif
