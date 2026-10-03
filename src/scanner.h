#ifndef SCANNER_H
#define SCANNER_H

typedef enum
{
 Token_ID,
 Token_INT,
 Token_DOUBLE,
 TOKEN_STRING,
 TOKEN_PLUS,
 TOKEN_MINUS,
 TOKEN_IF,
 TOkEN_ELSE,
 TOKEN_WHILE,
 TOKEN_FOR,
 TOKEN_AND,
 TOKEN_OR,
 TOKEN_TRUE,
 TOKEN_FALSE,
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
 TOKEN_LBRACKET,
 TOKEN_RBRACKET,
 TOKEN_LBRACE,
 TOKEN_RBRACE,
 TOKEN_ASSIGN
} TokenType;

typedef struct 
{
    TokenType type;
    union {
        int int_val;
        double double_val;
        char *str_val; 
    } value;
    /*to do*/
}Token;

typedef struct
{
    char *str;
    int length;
    int capacity;
}TokenString;



#ifdef __cplusplus
extern "C" {
#endif

Token scanner_get_next_token(void);

#ifdef __cplusplus
}
#endif

#endif