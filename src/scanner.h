#ifndef SCANNER_H
#define SCANNER_H

typedef enum {
    Token_KW,
    Token_ID,
    Token_OP,
    Token_INT,
    Token_DOUBLE,
    TOKEN_STRING,

} TokenType;

typedef struct {
    TokenType type;
    union {
        int int_val;
        double double_val;
        char *str_val;
    } value;
    /*to do*/
} Token;


#ifdef __cplusplus
extern "C" {
#endif

Token scanner_get_next_token(void);

#ifdef __cplusplus
}
#endif

#endif