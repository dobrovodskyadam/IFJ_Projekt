#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tac_gen.h"
#include "error.h"

static int temp_counter = 1;
static int label_counter = 1;
static int current_func_end_label = -1;

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
Operand op_bool(bool val)
{
    Operand op;
    op.kind = OP_BOOL;
    op.data.bool_val = val;
    return op;
}
Operand op_nil(void)
{
    Operand op;
    op.kind = OP_NIL;
    return op;
}

static char *copy_str(const char *s)
{
    char *p = malloc(strlen(s) + 1);
    if(p == NULL)
        error_exit(ERR_INTERNAL, 0, "Memory allocation error");
    strcpy(p,s);
    return p;
}

static bool is_value_node(Node *n)
{
    switch (n->kind)
    {
    case NODE_INT: case NODE_DOUBLE: case NODE_STRING: case NODE_BOOLEAN: case NODE_UNIT:
    case NODE_ID: case NODE_EXPRESSION: case NODE_BINARY_OPERATOR:
    case NODE_UNARY_OPERATOR: case NODE_FUNCTION_CALL:
        return true;
    default:
        return false;
    }
}
void tac_gen_statement(Node *node, TacList *list); 
Operand tac_gen_exp(Node *node, TacList *list);

static Operand gen_body_value(Node *body, TacList *list)
{
    if(body == NULL)
    {
        return op_none();
    }
    if(body->kind != NODE_BLOCK)
    {
        if(is_value_node(body))
            return tac_gen_exp(body,list);
        tac_gen_statement(body,list);
        return op_none();
    }

    NodeList *st = &body->data.block_data.statements;
    if(st->count == 0)
    {
        return op_none();
    }
    for(unsigned int i = 0; i + 1 < st->count; i++)
    {
        tac_gen_statement(st->list[i],list);
    }
    Node *last = st->list[st->count - 1];
    if(is_value_node(last))
    {
        return tac_gen_exp(last,list);
    }
    tac_gen_statement(last, list);
    return op_none();
}

int new_temp(void) {
    return temp_counter++;
}
int new_label(void) {
    return label_counter++;
}

static Operand coerce(Operand op, const Type *from, DataType to, TacList *list)
{
    if (from->kind == TYPE_INT && to == TYPE_DOUBLE)
    {
        if(op.kind == OP_INT)
        {
            return op_double((double)op.data.int_val);
        }
        Operand tmp = op_temp(new_temp());
        add_instr(list,TAC_INT2FLOAT,tmp,op,op_none());
        return tmp;
    }
    return op;
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
    case NODE_BOOLEAN:
        return op_bool(node->data.bool_value);
    case NODE_BINARY_OPERATOR:
    {
        TokenType op = node->data.bin_op_data.op;
        Node *left = node->data.bin_op_data.left;
        Node *right = node->data.bin_op_data.right;
        
        if (op == TOKEN_DIV)
        {
            Operand l_operand = tac_gen_exp(node->data.bin_op_data.left,list);
            Operand r_operand = tac_gen_exp(node->data.bin_op_data.right,list);

            if(left->type.kind == TYPE_DOUBLE || right->type.kind == TYPE_DOUBLE)
            {
                l_operand = coerce(l_operand,&left->type,TYPE_DOUBLE,list);
                r_operand = coerce(r_operand,&right->type,TYPE_DOUBLE,list);
            }

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

            add_instr(list,TAC_MOVE,tem_res,op_bool(true),op_none());
            add_instr(list,TAC_JUMP,op_none(),op_none(),op_label(label_end));

            add_instr(list,TAC_LABEL,op_label(label_false),op_none(),op_none());
            add_instr(list,TAC_MOVE,tem_res,op_bool(false),op_none());

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
            add_instr(list,TAC_MOVE,tem_res,op_bool(true),op_none());   
            add_instr(list,TAC_JUMP,op_none(),op_none(),op_label(label_end));

            add_instr(list,TAC_LABEL,op_label(label_false),op_none(),op_none());
            add_instr(list,TAC_MOVE,tem_res,op_bool(false),op_none());

            add_instr(list,TAC_LABEL,op_label(label_end),op_none(),op_none());

            return tem_res;
        }
        

        Operand l_operand = tac_gen_exp(node->data.bin_op_data.left, list);
        Operand r_operand = tac_gen_exp(node->data.bin_op_data.right,list);
        
        if (left->type.kind == TYPE_DOUBLE || right->type.kind == TYPE_DOUBLE)
        {
            l_operand = coerce(l_operand, &left->type, TYPE_DOUBLE, list);
            r_operand = coerce(r_operand, &right->type, TYPE_DOUBLE, list);
        }

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
            TacOp tac_op = token_to_tac_op(op);
            if (op == TOKEN_PLUS && left->type.kind == TYPE_STRING)
                tac_op = TAC_CONCAT;
            add_instr(list, tac_op, temp_res, l_operand, r_operand);
            return temp_res;
        }
    }
    case NODE_FUNCTION_CALL:
    {
        unsigned int argc = node->data.call_data.arguments.count;
        Operand *args = NULL;
        if (strcmp(node->data.call_data.name, "print") == 0)
        {
            Node *arg = node->data.call_data.arguments.list[0];
            Operand val = (arg->type.kind == TYPE_UNIT) ? op_string("()") : tac_gen_exp(arg, list);
            add_instr(list, TAC_WRITE, op_none(), val, op_none());
            return op_nil();
        }

        if(argc > 0)
        {
            args = malloc(argc*sizeof(Operand));
            if(args == NULL)
            {
                error_exit(ERR_INTERNAL,0,"Memmory allocation error");
            }
        }

        for(unsigned int i = 0; i < argc; i++)
        {
            args[i] = tac_gen_exp(node->data.call_data.arguments.list[i], list);
        }

        for(unsigned int i = 0; i < argc; i++)
        {
            add_instr(list, TAC_PARAM, args[i],op_none(),op_none());
        }
        free(args);
        
        Operand tmp_re = op_temp(new_temp());
        Operand func_name = op_var(node->data.call_data.name);
        
        add_instr(list,TAC_CALL,tmp_re,func_name,op_none());

        return tmp_re;
    }
    case NODE_UNARY_OPERATOR:
    {
        if (node->data.un_op_data.op == TOKEN_MINUS)
        {
             Operand zero = (node->data.un_op_data.operand->type.kind == TYPE_DOUBLE)
                           ? op_double(0.0) : op_int(0);        
            Operand val = tac_gen_exp(node->data.un_op_data.operand,list);
            Operand temp_re = op_temp(new_temp());
            
            add_instr(list,TAC_SUB,temp_re,zero,val);
            return temp_re;
        }
        else if (node->data.un_op_data.op == TOKEN_PLUS)
        {
            Operand zero = (node->data.un_op_data.operand->type.kind == TYPE_DOUBLE)
                           ? op_double(0.0) : op_int(0);
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
    case NODE_UNIT:
        return op_nil();
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
        NodeList *defs = &node->data.program_data.definitions;
        const char *main_name = NULL;

        for(unsigned int i = 0; i < node->data.program_data.definitions.count; i++)
        {
           Node *def = node->data.program_data.definitions.list[i];
           if(def != NULL && def->kind == NODE_FUNCTION_DEF && def->data.function_def_data.is_main)
           {
                main_name = def->data.function_def_data.name;
                break;
           }
        }
        int program_end = new_label();
        if(main_name != NULL)
            add_instr(list,TAC_CALL,op_none(),op_var(main_name),op_none());
        add_instr(list, TAC_JUMP,op_none(),op_none(),op_label(program_end));

        for(unsigned int i = 0; i < defs->count; i++)
        {
            tac_gen_statement(defs->list[i],list);
        }
        add_instr(list, TAC_LABEL,op_label(program_end),op_none(),op_none());
        break;
    }
    case NODE_FUNCTION_DEF:
    {
        FunctionDefData *fn = &node->data.function_def_data;
        int end_label = new_label();
        current_func_end_label = end_label;

        add_instr(list, TAC_LABEL, op_var(fn->name), op_none(), op_none());


        add_instr(list, TAC_MOVE, op_var("%retval"), op_nil(), op_none());


        for (unsigned int i = 0; i < fn->params.count; i++)
        {
            Node *param_node = fn->params.list[i];
            char slot_buf[32];
            snprintf(slot_buf, sizeof(slot_buf), "%%%u", i + 1);
            add_instr(list, TAC_MOVE, op_var(param_node->data.param_data.name),
                      op_var(copy_str(slot_buf)), op_none());
        }

        Operand last = gen_body_value(fn->body, list);
        if (last.kind != OP_NONE && fn->return_type.kind != TYPE_UNIT)
            add_instr(list, TAC_MOVE, op_var("%retval"), last, op_none());

        add_instr(list, TAC_LABEL, op_label(end_label), op_none(), op_none());
        add_instr(list, TAC_RETURN, op_none(), op_none(), op_none());

        current_func_end_label = -1;
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
            Node *value = node->data.var_decl_data.value;
            Operand val_op = tac_gen_exp(value, list);
            val_op = coerce(val_op, &value->type,
                        node->data.var_decl_data.type.kind, list);
            add_instr(list, TAC_MOVE, var_op, val_op, op_none());
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
    case NODE_RETURN:
    {
        if (node->data.return_data.value != NULL)
        {
            Operand ret_val = tac_gen_exp(node->data.return_data.value, list);
            add_instr(list, TAC_MOVE, op_var("%retval"), ret_val, op_none());
        }
        if (current_func_end_label != -1)
        {
            add_instr(list, TAC_JUMP, op_none(), op_none(), op_label(current_func_end_label));
        }
        break;
    }
    default:
        tac_gen_exp(node, list);
        break;
    }
}


static void print_operand(Operand op, FILE *output) {
    switch (op.kind) {
        case OP_INT:
            fprintf(output, "%d", op.data.int_val);
            break;
        case OP_DOUBLE:
            fprintf(output, "%f", op.data.double_val);
            break;
        case OP_VAR:
            fprintf(output, "%s", op.data.name);
            break;
        case OP_TEMP:
            fprintf(output, "T%d", op.data.temp_id);
            break;
        case OP_LABEL:
            fprintf(output, "L%d", op.data.label_id);
            break;
        case OP_STRING:
            fprintf(output, "%s", op.data.str_val);
            break;
        case OP_BOOL:
            fprintf(output, "%s", op.data.bool_val ? "true" : "false");
            break;
        case OP_NONE:
        default:
            break;
    }
}

void tac_code_gen(Node *root, FILE *output) {
    TacList list;
    tac_list_init(&list);

    tac_gen_statement(root, &list);

    for (int i = 0; i < list.count; i++) {
        TACInstr *instr = &list.items[i];

        switch (instr->op) {
            case TAC_LABEL:
                print_operand(instr->res, output);
                fprintf(output, ":\n");
                break;
            case TAC_JUMP:
                fprintf(output, "goto ");
                print_operand(instr->b, output);
                fprintf(output, "\n");
                break;
            case TAC_JUMP_IF_FALSE:
                fprintf(output, "if_false ");
                print_operand(instr->a, output);
                fprintf(output, " goto ");
                print_operand(instr->b, output);
                fprintf(output, "\n");
                break;
            case TAC_NOT:
                print_operand(instr->res, output);
                fprintf(output, " = not ");
                print_operand(instr->a, output);
                fprintf(output, "\n");
                break;
            case TAC_MOVE:
                print_operand(instr->res, output);
                fprintf(output, " = ");
                print_operand(instr->a, output);
                fprintf(output, "\n");
                break;
            case TAC_PARAM:
                fprintf(output, "param ");
                print_operand(instr->res, output); 
                fprintf(output, "\n");
                break;
            case TAC_CALL:
                print_operand(instr->res, output);
                fprintf(output, " = call ");
                print_operand(instr->a, output);
                fprintf(output, "\n");
                break;
            case TAC_JUMP_IF_TRUE:
                fprintf(output, "if_true ");
                print_operand(instr->a, output);
                fprintf(output, " goto ");
                print_operand(instr->b, output);
                fprintf(output, "\n");
                break;
            default:
                print_operand(instr->res, output);
                fprintf(output, " = ");
                print_operand(instr->a, output);
                
                const char *op_str = "+";
                if (instr->op == TAC_SUB) op_str = "-";
                else if (instr->op == TAC_MUL) op_str = "*";
                else if (instr->op == TAC_IDIV) op_str = "/";
                else if (instr->op == TAC_DIV) op_str = "/"; 
                else if (instr->op == TAC_LT) op_str = "<";
                else if (instr->op == TAC_GT) op_str = ">";
                else if (instr->op == TAC_EQ) op_str = "==";

                fprintf(output, " %s ", op_str);
                print_operand(instr->b, output);
                fprintf(output, "\n");
                break;
        }
    }

    tac_list_free(&list);
}

void tac_gen_reset(void) {
    temp_counter = 1;
    label_counter = 1;
    current_func_end_label = -1;
}

