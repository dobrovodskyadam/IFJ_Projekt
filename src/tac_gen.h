#ifndef TAC_GEN_H
#define TAC_GEN_H
#include <stdio.h>
#include "ast.h"


typedef enum {
    TAC_MOVE, TAC_ADD, TAC_SUB, TAC_MUL, TAC_IDIV,
    TAC_DIV,TAC_LT, TAC_GT, TAC_EQ,TAC_NOT,
    TAC_LABEL, TAC_JUMP, TAC_JUMP_IF_FALSE, TAC_JUMP_IF_TRUE,
    TAC_WRITE, TAC_PARAM, TAC_CALL, TAC_RETURN,TAC_CONCAT,TAC_INT2FLOAT,
    TAC_READ,TAC_STRLEN,TAC_FLOAT2INT,TAC_INT2STR,TAC_FLOAT2STR,TAC_STRI2INT,
    TAC_INT2CHAR,TAC_GETCHAR,TAC_EXIT,TAC_NEWARRAY,TAC_ARRAYGET,TAC_ARRAYSET
} TacOp;

typedef enum {OP_NONE, OP_VAR, OP_TEMP, OP_INT, OP_DOUBLE, OP_STRING, OP_LABEL,OP_BOOL,OP_NIL }OperandKind;

typedef struct {
    OperandKind kind;       
    union {
        const char *name;     
        int temp_id;          
        int int_val;          
        double double_val;    
        const char *str_val;  
        int label_id;  
        bool bool_val;       
    } data;                   
} Operand;

typedef struct 
{
    TacOp op;
    Operand res;
    Operand a;
    Operand b;
}TACInstr;

typedef struct 
{
    TACInstr *items;
    int count;
    int capacity;
}TacList;

typedef struct 
{
   const char *name;
   const char *unique;
   DataType type;
}ScopeVar;


void tac_gen_reset(void);

#endif