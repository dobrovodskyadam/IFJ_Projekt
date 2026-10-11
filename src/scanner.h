#ifndef SCANNER_H
#define SCANNER_H

typedef enum
{
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
 TOKEN_EOL
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

typedef struct
{
    int indent_stack[100];
    int stack_top;
    int dedent;
    int new_line;
}ScannerData;



#ifdef __cplusplus
extern "C" {
#endif

Token scanner_get_next_token(void);

#ifdef __cplusplus
}
#endif

#endif