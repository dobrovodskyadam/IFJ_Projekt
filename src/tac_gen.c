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


Operand tac_gen_exp(Node *node, TacList *list)
{
    if (node == NULL)
    {
        return op_none();
    }
    switch (node->kind)
    {
    case NODE_INT:
    {
        return op_int(node->data.int_value);
    }
    case NODE_DOUBLE:
    {
        return op_double(node->data.double_value);
    }
    case NODE_STRING:
    {
        return op_string(node->data.string_val);
    }
    case NODE_ID:
    {
        return op_var(node->data.variable_name);
    }
    case NODE_EXPRESSION:
    {
        return tac_gen_exp(node->data.expression_data,list);
    }
    case NODE_BINARY_OPERATOR:
    {
        TokenType op = node->data.bin_op_data.op;
        
        if (op == TOKEN_DIV)
        {
            Operand l_operand = tac_gen_exp(node->data.bin_op_data.left,list);
            Operand r_operand = tac_gen_exp(node->data.bin_op_data.right,list);

            Operand temp_res = op_temp(new_temp());

            if(node->data.bin_op_data.left->type.kind == TYPE_INT && node->data.bin_op_data.right->type.kind == TYPE_INT)
            {
                add_instr(list,TAC_IDIV,temp_res,l_operand,r_operand);
            }
            else
                add_instr(list,TAC_DIV,temp_res,l_operand,r_operand);

            return temp_res;
        }

        if(op == TOKEN_AND)
        {
            int label_false = new_label();
            int label_end = new_label();

            Operand tem_res = op_temp(new_temp());
            Operand l_side = tac_gen_exp(node->data.bin_op_data.left,list);
            add_instr(list,TAC_JUMP_IF_FALSE,op_none(),l_side,op_label(label_false));

            Operand r_side = tac_gen_exp(node->data.bin_op_data.right,list);
            add_instr(list,TAC_JUMP_IF_FALSE,op_none(),r_side,op_label(label_false));

            add_instr(list,TAC_MOVE,tem_res,op_int(1),op_none());
            add_instr(list,TAC_JUMP,op_none(),op_none(),op_label(label_end));

            add_instr(list,TAC_LABEL,op_label(label_false),op_none(),op_none());
            add_instr(list,TAC_MOVE,tem_res,op_int(0),op_none());

            add_instr(list,TAC_LABEL,op_label(label_end),op_none(),op_none());

           return tem_res;

        }
        else if (op == TOKEN_OR)
        {
            int label_true = new_label();
            int label_false = new_label();
            int label_end = new_label();

            Operand tem_res = op_temp(new_temp());
            Operand l_side = tac_gen_exp(node->data.bin_op_data.left,list);
            add_instr(list,TAC_JUMP_IF_TRUE,op_none(),l_side,op_label(label_true));

            Operand r_side = tac_gen_exp(node->data.bin_op_data.right,list);
            add_instr(list,TAC_JUMP_IF_FALSE,op_none(),r_side,op_label(label_false));

            add_instr(list,TAC_LABEL,op_label(label_true),op_none(),op_none());
            add_instr(list,TAC_MOVE,tem_res,op_int(1),op_none());   
            add_instr(list,TAC_JUMP,op_none(),op_none(),op_label(label_end));

            add_instr(list,TAC_LABEL,op_label(label_false),op_none(),op_none());
            add_instr(list,TAC_MOVE,tem_res,op_int(0),op_none());

            add_instr(list,TAC_LABEL,op_label(label_end),op_none(),op_none());

            return tem_res;
        }
        

        Operand l_operand = tac_gen_exp(node->data.bin_op_data.left, list);
        Operand r_operand = tac_gen_exp(node->data.bin_op_data.right,list);

        if(op == TOKEN_NEQ || op == TOKEN_GR_EQ || op == TOKEN_LS_EQ )
        {
            TacOp base_op;
            if (op == TOKEN_LS_EQ )
            {
                base_op = TAC_GT;
            }
            else if (op == TOKEN_GR_EQ)
            {
                base_op = TAC_LT;
            }
            else
                base_op = TAC_EQ;
            
            Operand temp_cmp = op_temp(new_temp());
            add_instr(list,base_op,temp_cmp,l_operand,r_operand);

            Operand temp_res = op_temp(new_temp());
            add_instr(list,TAC_NOT,temp_res,temp_cmp,op_none());
            return temp_res;
        }
        else
        {
            Operand temp_res = op_temp(new_temp());
            TacOp tac_op = token_to_tac_op(node->data.bin_op_data.op);
            add_instr(list, tac_op, temp_res, l_operand, r_operand);
            return temp_res;
        }
    }
    case NODE_FUNCTION_CALL:
    {
        for(unsigned int i = 0; i < node->data.call_data.arguments.count; i++ )
        {
            Operand tmp_operand = tac_gen_exp(node->data.call_data.arguments.list[i],list);
            add_instr(list,TAC_PARAM,tmp_operand,op_none(),op_none());
        }
        
        Operand tmp_re = op_temp(new_temp());
        Operand func_name = op_var(node->data.call_data.name);
        
        add_instr(list,TAC_CALL,tmp_re,func_name,op_none());

        return tmp_re;
    }
    case NODE_UNARY_OPERATOR:
    {
        if (node->data.un_op_data.op == TOKEN_MINUS)
        {
            Operand zero = op_int(0);
            Operand val = tac_gen_exp(node->data.un_op_data.operand,list);
            Operand temp_re = op_temp(new_temp());
            
            add_instr(list,TAC_SUB,temp_re,zero,val);
            return temp_re;
        }
        else if (node->data.un_op_data.op == TOKEN_PLUS)
        {
            Operand zero = op_int(0);
            Operand val = tac_gen_exp(node->data.un_op_data.operand,list);
            Operand temp_re = op_temp(new_temp());

            add_instr(list,TAC_ADD,temp_re,zero,val);
            return temp_re;
        }
        else
        {
            Operand temp_re = op_temp(new_temp());
            Operand val = tac_gen_exp(node->data.un_op_data.operand,list);
            add_instr(list,TAC_NOT,temp_re,val,op_none());
            return temp_re;
        }
    }
    default:
        break;
    }
    return op_none();
}

void tac_gen_statement(Node *node, TacList *list)
{
    if (node == NULL)
        return;

    switch (node->kind)
    {
    case NODE_PROGRAM:
    {
        for(unsigned int i = 0; i < node->data.program_data.definitions.count; i++)
        {
            tac_gen_statement(node->data.program_data.definitions.list[i],list);
        }
        break;
    }
    case NODE_FUNCTION_DEF:
    {
        tac_gen_statement(node->data.function_def_data.body, list);
        break;
    }
    case NODE_ASSIGN:
    {
        Operand var_op = op_var(node->data.assign_data.name);
        Operand op_val = tac_gen_exp(node->data.assign_data.value, list);

        add_instr(list,TAC_MOVE,var_op,op_val,op_none());
        break;
    }
    case NODE_VARIABLE_DECLARATION:
    {
        Operand var_op = op_var(node->data.var_decl_data.name);
        if(node->data.var_decl_data.value != NULL)
        {
            Operand val_op = tac_gen_exp(node->data.var_decl_data.value, list);
            add_instr(list,TAC_MOVE,var_op,val_op,op_none());
        }
        break;
    }
    case NODE_BLOCK:
    {
        for (unsigned int i = 0; i < node->data.block_data.statements.count; i++)
        {
            tac_gen_statement(node->data.block_data.statements.list[i], list);
        }
        break;
    }
    case NODE_IF_STATEMENT:
    {
        int label_then = new_label();

        Operand cond = tac_gen_exp(node->data.if_data.condition,list);

        if (node->data.if_data.else_part == NULL)
        {
            add_instr(list,TAC_JUMP_IF_FALSE,op_none(),cond,op_label(label_then));
            tac_gen_statement(node->data.if_data.then_part, list);
            add_instr(list, TAC_LABEL, op_label(label_then), op_none(), op_none());
        }
        else
        {
            int label_else = new_label();
            add_instr(list, TAC_JUMP_IF_FALSE, op_none(), cond, op_label(label_then));
            tac_gen_statement(node->data.if_data.then_part, list);
            add_instr(list, TAC_JUMP, op_none(), op_none(), op_label(label_else));
            add_instr(list, TAC_LABEL, op_label(label_then), op_none(), op_none());
            tac_gen_statement(node->data.if_data.else_part, list);
            add_instr(list, TAC_LABEL, op_label(label_else), op_none(), op_none());
        }
        break;
    }
    case NODE_WHILE_CYCLE:
    {
        int label_start = new_label();
        int label_end = new_label();

        add_instr(list, TAC_LABEL, op_label(label_start), op_none(), op_none());
        Operand cond = tac_gen_exp(node->data.while_data.condition, list);
        add_instr(list, TAC_JUMP_IF_FALSE, op_none(), cond, op_label(label_end));
        tac_gen_statement(node->data.while_data.body, list);
        add_instr(list, TAC_JUMP, op_none(), op_none(), op_label(label_start));
        add_instr(list, TAC_LABEL, op_label(label_end), op_none(), op_none());
        break;
    }
    default:
        break;
    }
}
