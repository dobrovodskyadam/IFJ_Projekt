#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tac_gen.h"
#include "error.h"

static int temp_counter = 1;
static int label_counter = 1;

void tac_list_init(TacList *list)
{
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void tac_list_free(TacList *list)
{
    if (list->items != NULL)
        free(list->items);

    tac_list_init(list);
}

void add_instr(TacList *list, TacOp op, Operand res, Operand a, Operand b)
{
    if(list->count == list->capacity)
    {
        size_t new_cap = list->capacity == 0 ? 16 : list->capacity * 2;
        TACInstr *p = realloc(list->items, new_cap * sizeof(TACInstr));
         if (p == NULL)
            error_exit(ERR_INTERNAL, 0, "Memory allocation error in emit");

        list->items = p;
        list->capacity = new_cap;

    }

    TACInstr *new = &list->items[list->count];
    list->count++;
    new->op = op;
    new->res = res;
    new->a = a;
    new->b = b;
}

Operand op_int(int val)
{
    Operand op;
    op.kind = OP_INT;
    op.data.int_val = val;
    return op;
}

Operand op_var(const char *name)
{
    Operand op;
    op.kind = OP_VAR;
    op.data.name = name;
    return op;
}

Operand op_temp(int id) {
    Operand op;
    op.kind = OP_TEMP;
    op.data.temp_id = id;
    return op;
}
Operand op_double(double val) {
    Operand op;
    op.kind = OP_DOUBLE;
    op.data.double_val = val;
    return op;
}
Operand op_string(const char *s) {
    Operand op;
    op.kind = OP_STRING;
    op.data.str_val = s;
    return op;
}
Operand op_none(void) {
    Operand op;
    op.kind = OP_NONE;
    return op;
}
Operand op_label(int id) {
    Operand op;
    op.kind = OP_LABEL;
    op.data.label_id = id;
    return op;
}
int new_temp(void) {
    return temp_counter++;
}
int new_label(void) {
    return label_counter++;
}

TacOp token_to_tac_op(TokenType op) {
    switch (op) {
        case TOKEN_PLUS:  return TAC_ADD;
        case TOKEN_MINUS: return TAC_SUB;
        case TOKEN_MUL:   return TAC_MUL;
        case TOKEN_DIV:   return TAC_IDIV; 
        case TOKEN_LS:    return TAC_LT;
        case TOKEN_GR:    return TAC_GT;
        case TOKEN_EQ:    return TAC_EQ;
        default:          
            return TAC_MOVE; 
    }
}
