#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tac_gen.h"
#include "error.h"

static int temp_counter = 1;
static int label_counter = 1;
static int current_func_end_label = -1;
static ScopeVar *scope_vars = NULL;
static int *scope_marks = NULL;
static int scope_cnt = 0;
static int scope_cap = 0;
static int mark_cnt = 0;
static int mark_cap = 0;
static int unique_counter = 1;
static const Type *current_ret_type = NULL;
static const Type g_unknown_type = {TYPE_UNKNOWN, NULL};
static NodeList *g_defs = NULL;

static void scope_push(void)
{
    if(mark_cnt == mark_cap)
    {
        mark_cap = mark_cap == 0 ? 16 : mark_cap * 2;
        int *p = realloc(scope_marks, mark_cap * sizeof(int));
        if(p==NULL)
        {
            error_exit(ERR_INTERNAL, 0, "Memory allocation error in scope_push");
        } 
        scope_marks = p;
    }
    scope_marks[mark_cnt++] = scope_cnt;
}

static void scope_pop(void)
{
    if(mark_cnt > 0)
    {
        scope_cnt = scope_marks[--mark_cnt];
    }
}

static const char *scope_declare(const char *name, DataType type)
{
    if(scope_cnt == scope_cap)
    {
        scope_cap = scope_cap == 0 ? 32 : scope_cap * 2;
        ScopeVar *p = realloc(scope_vars,scope_cap*sizeof(ScopeVar));
        if(p == NULL)
            error_exit(ERR_INTERNAL, 0, "Memory allocation error in scope_declare");
        
        scope_vars = p;
    }
    char *uni = malloc(strlen(name) +16 );
    if(uni == NULL)
        error_exit(ERR_INTERNAL, 0, "Memory allocation error in scope_declare");
    sprintf(uni, "%s$%d", name, unique_counter++);
    scope_vars[scope_cnt].name = name;
    scope_vars[scope_cnt].unique = uni;
    scope_vars[scope_cnt].type = type;
    scope_cnt++;
    return uni;
}

static const char *scope_lookup(const char *name)
{
    for(int i = scope_cnt - 1; i >=0; i-- )
    {
        if(strcmp(scope_vars[i].name, name) == 0)
            return scope_vars[i].unique;
    }
    return name;
}

static void scope_reset(void)
{
    scope_cnt = 0;
    mark_cnt  = 0;
}

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
    case NODE_UNARY_OPERATOR: case NODE_FUNCTION_CALL: case NODE_IF_VALUE:
        return true;
    case NODE_IF_STATEMENT:
        return n->data.if_data.else_part != NULL
            && n->type.kind != TYPE_UNIT
            && n->type.kind != TYPE_UNKNOWN
            && n->type.kind != TYPE_UNDEF;
    default:
        return false;
    }
}
static FunctionDefData *find_fn(const char *name)
{
    if(g_defs == NULL)
        return NULL;

    for(unsigned int i = 0; i < g_defs->count; i++)
    {
        Node *n = g_defs->list[i];
        if(n != NULL && n->kind == NODE_FUNCTION_DEF && strcmp(n->data.function_def_data.name, name) == 0)
        {
            return &n->data.function_def_data;
        }
    }
    return NULL;
}

void tac_gen_statement(Node *node, TacList *list); 
Operand tac_gen_exp(Node *node, TacList *list);
static Operand coerce(Operand op, const Type *from, DataType to, TacList *list);
int new_temp(void);
int new_label(void);

static Operand gen_body_value(Node *body,TacList *list,  DataType want)
{
    if(body == NULL)
    {
        return op_none();
    }
    if(body->kind != NODE_BLOCK)
    {
        if(is_value_node(body))
           return coerce(tac_gen_exp(body, list), &body->type, want, list);
        tac_gen_statement(body,list);
        return op_none();
    }
    NodeList *st = &body->data.block_data.statements;
    if(st->count == 0)
    {
        return op_none();
    }
    scope_push();
    for(unsigned int i = 0; i + 1 < st->count; i++)
    {
        tac_gen_statement(st->list[i],list);
    }

    Node *last = st->list[st->count - 1];
    Operand r = op_none();
    if (is_value_node(last))
        r = coerce(tac_gen_exp(last, list), &last->type, want, list);
    else
        tac_gen_statement(last, list);
    scope_pop();
    return r;
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

static DataType scope_type(const char *name)
{
    for(int i = scope_cnt - 1; i >=0; i-- )
    {
        if(strcmp(scope_vars[i].name, name) == 0)
        {
            return scope_vars[i].type;
        }
    }
    return TYPE_UNKNOWN;
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

static Operand print_un(TacList *list, TacOp op, Operand a)
{
    Operand tmp = op_temp(new_temp());
    add_instr(list,op,tmp,a,op_none());
    return tmp;
}
static Operand print_bin(TacList *list, TacOp op, Operand a, Operand b)
{
    Operand tmp = op_temp(new_temp());
    add_instr(list,op,tmp,a,b);
    return tmp;
}
static Operand arr_get(TacList *list, Operand arr, Operand idx )
{
    Operand tmp = op_temp(new_temp());
    add_instr(list,TAC_ARRAYGET,tmp,arr,idx);
    return tmp;
}
static Operand arr_len(TacList *list,Operand arr)
{
    return arr_get(list,arr,op_int(0));
}
static void arr_set(TacList *list, Operand arr, Operand idx, Operand val)
{
    add_instr(list, TAC_ARRAYSET, arr, idx, val);
}
static Operand list_alloc(TacList *list, Operand op)
{
    Operand size = print_bin(list,TAC_ADD,op,op_int(1));
    Operand arr = op_temp(new_temp());
    add_instr(list,TAC_NEWARRAY,arr,size,op_none());
    arr_set(list,arr,op_int(0),op);
    return arr;
}
static void list_copy(TacList *list,Operand dst,Operand dpos,Operand src,Operand spos,Operand count)
{
    Operand tmp = op_temp(new_temp());
    int l_loop = new_label();
    int l_done = new_label();

    add_instr(list, TAC_MOVE, tmp, op_int(0), op_none());
    add_instr(list, TAC_LABEL, op_label(l_loop), op_none(), op_none());
    Operand c = print_bin(list, TAC_LT, tmp, count);
    add_instr(list, TAC_JUMP_IF_FALSE, op_none(), c, op_label(l_done));
    Operand si = print_bin(list, TAC_ADD, spos, tmp);
    Operand v  = arr_get(list, src, si);
    Operand di = print_bin(list, TAC_ADD, dpos, tmp);
    arr_set(list, dst, di, v);
    add_instr(list, TAC_ADD, tmp, tmp, op_int(1));
    add_instr(list, TAC_JUMP, op_none(), op_none(), op_label(l_loop));
    add_instr(list, TAC_LABEL, op_label(l_done), op_none(), op_none());
}
static Operand gen_list_index(Node *node, TacList *list)
{
    Operand arr = op_var(scope_lookup(node->data.call_data.name));
    Operand idx = tac_gen_exp(node->data.call_data.arguments.list[0], list);
    Operand len = arr_len(list, arr);
    int l_err = new_label();
    int l_ok  = new_label();

    Operand neg = print_bin(list, TAC_LT, idx, op_int(0));
    add_instr(list, TAC_JUMP_IF_TRUE, op_none(), neg, op_label(l_err));
    Operand in = print_bin(list, TAC_LT, idx, len);
    add_instr(list, TAC_JUMP_IF_TRUE, op_none(), in, op_label(l_ok));
    add_instr(list, TAC_LABEL, op_label(l_err), op_none(), op_none());
    add_instr(list, TAC_EXIT, op_none(), op_int(40), op_none());
    add_instr(list, TAC_LABEL, op_label(l_ok), op_none(), op_none());

    Operand slot = print_bin(list, TAC_ADD, idx, op_int(1));
    return arr_get(list,arr,slot);

}
static Operand gen_slice(TacList *list, Operand src, Operand from, Operand until)
{
    Operand len = arr_len(list, src);
    Operand lo = op_temp(new_temp());
    Operand hi = op_temp(new_temp());
    int l1 = new_label();
    int l2 = new_label();
    int l3 = new_label();

    add_instr(list, TAC_MOVE, lo, from, op_none());
    add_instr(list, TAC_MOVE, hi, until, op_none());

    Operand c1 = print_bin(list, TAC_LT,lo,op_int(0));
    add_instr(list, TAC_JUMP_IF_FALSE, op_none(), c1, op_label(l1));
    add_instr(list, TAC_MOVE, lo, op_int(0), op_none());
    add_instr(list, TAC_LABEL, op_label(l1), op_none(), op_none());

    Operand c2 = print_bin(list,TAC_GT, hi, len);
    add_instr(list, TAC_JUMP_IF_FALSE, op_none(), c2, op_label(l2));
    add_instr(list, TAC_MOVE, hi, len, op_none());
    add_instr(list, TAC_LABEL, op_label(l2), op_none(), op_none());

    Operand c3 = print_bin(list, TAC_LT, hi, lo);                  
    add_instr(list, TAC_JUMP_IF_FALSE, op_none(), c3, op_label(l3));
    add_instr(list, TAC_MOVE, hi, lo, op_none());
    add_instr(list, TAC_LABEL, op_label(l3), op_none(), op_none());

    Operand n = print_bin(list, TAC_SUB, hi,lo);
    Operand dst = list_alloc(list, n);
    Operand sp  = print_bin(list, TAC_ADD, lo, op_int(1));
    list_copy(list, dst, op_int(1), src, sp, n);
    return dst;

}
static Operand gen_list_concat(TacList *list, Operand a, Operand b)
{
    Operand la  = arr_len(list, a);
    Operand lb  = arr_len(list, b);
    Operand n   = print_bin(list, TAC_ADD, la, lb);
    Operand dst = list_alloc(list, n);
    list_copy(list, dst, op_int(1), a, op_int(1), la);
    Operand off = print_bin(list, TAC_ADD, la, op_int(1));
    list_copy(list, dst, off, b, op_int(1), lb);
    return dst;
}
static Operand gen_to_str(TacList *list, Operand op, const Type *t)
{
    switch (t->kind)
    {
        case TYPE_INT: return print_un(list, TAC_INT2STR, op);
        case TYPE_DOUBLE: return print_un(list, TAC_FLOAT2STR, op);
        case TYPE_STRING: return op;
        case TYPE_UNIT: return op_string("()");
        case TYPE_BOOL:
        {
            Operand tmp = op_temp(new_temp());
            int l_false = new_label();
            int l_end = new_label();

            add_instr(list, TAC_JUMP_IF_FALSE,op_none(),op,op_label(l_false));
            add_instr(list, TAC_MOVE, tmp, op_string("true"), op_none());
            add_instr(list, TAC_JUMP, op_none(), op_none(), op_label(l_end));
            add_instr(list, TAC_LABEL, op_label(l_false), op_none(), op_none());
            add_instr(list, TAC_MOVE, tmp, op_string("false"), op_none());
            add_instr(list, TAC_LABEL, op_label(l_end), op_none(), op_none());
            return tmp;
        }
        case TYPE_LIST: 
        {
            const Type *et = &g_unknown_type;
            if (t->elem != NULL)
            {
                et = t->elem;
            }
            Operand res = op_temp(new_temp());
            Operand i = op_temp(new_temp());
            Operand len = arr_len(list, op);
            int l_loop = new_label();
            int l_nosep = new_label();
            int l_done = new_label();

            add_instr(list, TAC_MOVE, res, op_string("List("), op_none());
            add_instr(list, TAC_MOVE, i, op_int(0), op_none());
            add_instr(list, TAC_LABEL, op_label(l_loop), op_none(), op_none());
            Operand c = print_bin(list, TAC_LT, i, len);
            add_instr(list, TAC_JUMP_IF_FALSE, op_none(), c, op_label(l_done));
            Operand first = print_bin(list, TAC_EQ, i, op_int(0));
            add_instr(list, TAC_JUMP_IF_TRUE, op_none(), first, op_label(l_nosep));
            add_instr(list, TAC_CONCAT, res, res, op_string(", "));
            add_instr(list, TAC_LABEL, op_label(l_nosep), op_none(), op_none());
            Operand slot = print_bin(list,TAC_ADD,i,op_int(1));
            Operand elem = arr_get(list, op, slot);
            Operand s = gen_to_str(list, elem, et);
            add_instr(list, TAC_CONCAT, res, res, s);
            add_instr(list, TAC_ADD, i, i, op_int(1));
            add_instr(list, TAC_JUMP, op_none(), op_none(), op_label(l_loop));
            add_instr(list, TAC_LABEL, op_label(l_done), op_none(), op_none());
            add_instr(list, TAC_CONCAT, res, res, op_string(")"));
            return res;
        }
        default:
            return op;
    }
}
static bool gen_builtin(Node *node, TacList *list, Operand *res);

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
        return op_var(scope_lookup(node->data.variable_name));
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
            Operand val = tac_gen_exp(arg, list);
            if(arg->type.kind == TYPE_UNIT)
            {
                val = op_string("()");
            }
            add_instr(list, TAC_WRITE, op_none(), val, op_none());
            return op_nil();
        }
        Operand builtin_res;
        if (gen_builtin(node, list, &builtin_res))
            return builtin_res;

        if(argc > 0)
        {
            args = malloc(argc*sizeof(Operand));
            if(args == NULL)
            {
                error_exit(ERR_INTERNAL,0,"Memmory allocation error");
            }
        }
        FunctionDefData *fd = find_fn(node->data.call_data.name);
        for(unsigned int i = 0; i < argc; i++)
        {
            Node *node_arg = node->data.call_data.arguments.list[i]; 
            args[i] = tac_gen_exp(node_arg,list);
            if(fd != NULL && i < fd->params.count)
            {
                args[i] = coerce(args[i],&node_arg->type, fd->params.list[i]->data.param_data.type.kind, list);
            }
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
    case NODE_IF_STATEMENT: 
    case NODE_IF_VALUE:
    {
        DataType want = node->type.kind;
        Operand res = op_temp(new_temp());
        int l_else = new_label();
        int l_end  = new_label();

        Operand cond = tac_gen_exp(node->data.if_data.condition, list);
        add_instr(list, TAC_JUMP_IF_FALSE, op_none(), cond, op_label(l_else));

        Operand then = gen_body_value(node->data.if_data.then_part, list, want);
        add_instr(list, TAC_MOVE, res, then.kind != OP_NONE ? then : op_nil(), op_none());
        add_instr(list, TAC_JUMP, op_none(), op_none(), op_label(l_end));

        add_instr(list, TAC_LABEL, op_label(l_else), op_none(), op_none());
        Operand els = gen_body_value(node->data.if_data.else_part, list, want);
        add_instr(list, TAC_MOVE, res, els.kind != OP_NONE ? els : op_nil(), op_none());

        add_instr(list, TAC_LABEL, op_label(l_end), op_none(), op_none());
        return res;
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
        NodeList *defs = &node->data.program_data.definitions;
        g_defs = defs;
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
        scope_reset(); 
        scope_push();
        FunctionDefData *fn = &node->data.function_def_data;
        int end_label = new_label();
        current_func_end_label = end_label;
        current_ret_type = &fn->return_type;

        add_instr(list, TAC_LABEL, op_var(fn->name), op_none(), op_none());


        add_instr(list, TAC_MOVE, op_var("%retval"), op_nil(), op_none());


        for (unsigned int i = 0; i < fn->params.count; i++)
        {
            Node *param_node = fn->params.list[i];
            char slot_buf[32];
            snprintf(slot_buf, sizeof(slot_buf), "%%%u", i + 1);
            const char *uni = scope_declare(param_node->data.param_data.name,param_node->data.param_data.type.kind);
            add_instr(list, TAC_MOVE, op_var(uni),
                      op_var(copy_str(slot_buf)), op_none());
        }

        Operand last = gen_body_value(fn->body, list, fn->return_type.kind);
        if (last.kind != OP_NONE && fn->return_type.kind != TYPE_UNIT)
            add_instr(list, TAC_MOVE, op_var("%retval"), last, op_none());

        add_instr(list, TAC_LABEL, op_label(end_label), op_none(), op_none());
        add_instr(list, TAC_RETURN, op_none(), op_none(), op_none());

        current_func_end_label = -1;
        current_ret_type = NULL;
        break;

    }
    case NODE_ASSIGN:
    {
        Node *value = node->data.assign_data.value;
        const char *name = node->data.assign_data.name;
        Operand val = tac_gen_exp(value, list);
        val = coerce(val, &value->type, scope_type(name), list);
        add_instr(list, TAC_MOVE, op_var(scope_lookup(name)), val, op_none());
        break;
    }
    case NODE_VARIABLE_DECLARATION:
    {
        //Operand var_op = op_var(node->data.var_decl_data.name);
        if(node->data.var_decl_data.value != NULL)
        {
            Node *value = node->data.var_decl_data.value;
            Operand val_op = tac_gen_exp(value, list);
            val_op = coerce(val_op, &value->type,
                        node->data.var_decl_data.type.kind, list);
            Operand var_op = op_var(scope_declare(node->data.var_decl_data.name,node->data.var_decl_data.type.kind));
            add_instr(list, TAC_MOVE, var_op, val_op, op_none());
        }
        break;
    }
    case NODE_BLOCK:
    {
        scope_push();
        for (unsigned int i = 0; i < node->data.block_data.statements.count; i++)
        {
            tac_gen_statement(node->data.block_data.statements.list[i], list);
        }
        scope_pop();
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
    case NODE_FOR_CYCLE:
    {
       ForData *fd = &node->data.for_data;
       Node *range = fd->range;
       Node *step_node = NULL;

       if(range->kind == NODE_BINARY_OPERATOR && range->data.bin_op_data.op == TOKEN_BY)
       {
            step_node = range->data.bin_op_data.right;
            range = range->data.bin_op_data.left;
       }
       bool inclusive = (range->data.bin_op_data.op == TOKEN_TO);

       Operand from = tac_gen_exp(range->data.bin_op_data.left, list);
       Operand lim = op_temp(new_temp());
       Operand to_val = tac_gen_exp(range->data.bin_op_data.right, list);

       add_instr(list,TAC_MOVE,lim,to_val,op_none());
       Operand step = op_int(1);
       if(step_node != NULL)
       {
            step = op_temp(new_temp());
            Operand step_val = tac_gen_exp(step_node,list);
            add_instr(list,TAC_MOVE,step,step_val,op_none());
       }
       scope_push();
       Operand iv = op_var(scope_declare(fd->name, TYPE_INT));
       add_instr(list, TAC_MOVE, iv, from, op_none());

       int l_start = new_label();
       int l_next = new_label();
       int l_end = new_label();

       if(step_node != NULL)
       {
            int l_ok = new_label();
            Operand is_zero = print_bin(list, TAC_EQ, step, op_int(0));
            add_instr(list,TAC_JUMP_IF_FALSE,op_none(),is_zero,op_label(l_ok));
            add_instr(list,TAC_EXIT,op_none(),op_int(40),op_none());
            add_instr(list,TAC_LABEL,op_label(l_ok),op_none(),op_none());
       }

       add_instr(list,TAC_LABEL, op_label(l_start),op_none(),op_none());

       TacOp jump = inclusive ? TAC_JUMP_IF_TRUE : TAC_JUMP_IF_FALSE;
       TacOp up   = inclusive ? TAC_GT : TAC_LT;
       TacOp down = inclusive ? TAC_LT : TAC_GT;

       if(step_node == NULL)
       {
            Operand c = print_bin(list,up,iv,lim);
            add_instr(list,jump,op_none(),c,op_label(l_end));
       }
       else
       {
            int l_neg = new_label();
            int l_body = new_label();
            Operand is_up = print_bin(list, TAC_GT, step, op_int(0));
            add_instr(list, TAC_JUMP_IF_FALSE, op_none(), is_up, op_label(l_neg));
            Operand c1 = print_bin(list, up, iv, lim);
            add_instr(list, jump, op_none(), c1, op_label(l_end));
            add_instr(list, TAC_JUMP, op_none(), op_none(), op_label(l_body));
            add_instr(list, TAC_LABEL, op_label(l_neg), op_none(), op_none());
            Operand c2 = print_bin(list, down, iv, lim);
            add_instr(list, jump, op_none(), c2, op_label(l_end));
            add_instr(list, TAC_LABEL, op_label(l_body), op_none(), op_none());
       }
       for(unsigned int k = 0; k < fd->guards.count; k++)
       {
            Operand g = tac_gen_exp(fd->guards.list[k], list);
            add_instr(list, TAC_JUMP_IF_FALSE, op_none(), g, op_label(l_next));  
       }

       tac_gen_statement(fd->body,list);
       add_instr(list, TAC_LABEL, op_label(l_next), op_none(), op_none());
       add_instr(list, TAC_ADD, iv, iv, step);
       add_instr(list, TAC_JUMP, op_none(), op_none(), op_label(l_start));
       add_instr(list, TAC_LABEL, op_label(l_end), op_none(), op_none());
       scope_pop();
       break;

    }
    case NODE_RETURN:
    {
        Node *value = node->data.return_data.value;
        if(value != NULL)
        {
            Operand val = tac_gen_exp(value,list);
            if(current_ret_type != NULL && current_ret_type->kind != TYPE_UNIT)
            {
                val = coerce(val,&value->type,current_ret_type->kind,list);
                add_instr(list,TAC_MOVE,op_var("%retval"),val,op_none());
            }
            
        }
        if(current_func_end_label != -1)
        {
            add_instr(list, TAC_JUMP,op_none(),op_none(),op_label(current_func_end_label));
        }
        break;
    }
    default:
        tac_gen_exp(node, list);
        break;
    }
}

static bool gen_builtin(Node *node, TacList *list, Operand *res)
{
    const char *name = node->data.call_data.name;
    NodeList *args = &node->data.call_data.arguments;

    if(strcmp(name,"readInt") == 0 || strcmp(name,"readDouble") == 0 || strcmp(name,"readLine") == 0)
    {
        const char *type = "int";
        Operand fallback = op_int(0);

        if(strcmp(name,"readDouble") == 0)
        {
            type = "float";
            fallback = op_double(0.0);
        }
        if(strcmp(name,"readLine") == 0)
        {
            type = "string";
            fallback = op_string("");
        }

        Operand tmp = op_temp(new_temp());
        add_instr(list, TAC_READ, tmp, op_string(type), op_none());
        Operand is_nil = print_bin(list,TAC_EQ,tmp,op_nil());
        int label_end = new_label();
        add_instr(list, TAC_JUMP_IF_FALSE, op_none(), is_nil, op_label(label_end));
        add_instr(list, TAC_MOVE, tmp, fallback, op_none());
        add_instr(list, TAC_LABEL, op_label(label_end), op_none(), op_none());
        *res = tmp;
        return true;

    }
    if(strcmp(name, "readBoolean") == 0)
    {
        Operand tmp = op_temp(new_temp());
        add_instr(list, TAC_READ, tmp, op_string("bool"), op_none());
        *res = tmp;
        return true;
    }

    if(strcmp(name, "strLength") == 0)
    {
        *res = print_un(list, TAC_STRLEN, tac_gen_exp(args->list[0], list));
        return true;
    }
    if(strcmp(name,"chr") == 0)
    {
        *res = print_un(list, TAC_INT2CHAR, tac_gen_exp(args->list[0], list));
        return true;
    }
    if(strcmp(name,"toInt") == 0)
    {
        Node *arg = args->list[0];
        Operand a = coerce(tac_gen_exp(arg, list), &arg->type, TYPE_DOUBLE, list);
        *res = print_un(list, TAC_FLOAT2INT, a);
        return true;
    }
    if(strcmp(name, "toStr") == 0)
    {
        Node *arg = args->list[0];
        Operand a = tac_gen_exp(arg, list);
        switch (arg->type.kind)
        {
        case TYPE_INT:
            *res = print_un(list,TAC_INT2STR,a);
            break;
        case TYPE_DOUBLE:
            *res = print_un(list, TAC_FLOAT2STR, a);
            break;
        case TYPE_STRING:
            *res = a;
            break;
        case TYPE_UNIT:
            *res = op_string("()"); 
            break;
        case TYPE_BOOL:
        {
            Operand tmp = op_temp(new_temp());
            int l_false = new_label();
            int l_end = new_label();

            add_instr(list, TAC_JUMP_IF_FALSE, op_none(), a, op_label(l_false));
            add_instr(list, TAC_MOVE, tmp, op_string("true"), op_none());
            add_instr(list, TAC_JUMP, op_none(), op_none(), op_label(l_end));
            add_instr(list, TAC_LABEL, op_label(l_false), op_none(), op_none());
            add_instr(list, TAC_MOVE, tmp, op_string("false"), op_none());
            add_instr(list, TAC_LABEL, op_label(l_end), op_none(), op_none());
            *res = tmp;
            break;
        }        
        default:
            *res = a; break; 
            break;
        }
        return true;
    }
    if(strcmp(name,"ord") == 0)
    {
        Operand s = tac_gen_exp(args->list[0],list);
        Operand i = tac_gen_exp(args->list[1], list);
        Operand tmp = op_temp(new_temp());
        int l_neg = new_label();
        int l_end = new_label();

        Operand len = print_un(list,TAC_STRLEN,s);
        Operand c1 = print_bin(list,TAC_LT,i,op_int(0));
        add_instr(list,TAC_JUMP_IF_TRUE,op_none(),c1,op_label(l_neg));
        Operand c2 = print_bin(list,TAC_LT,i,len);
        add_instr(list,TAC_JUMP_IF_FALSE,op_none(),c2,op_label(l_neg));

        add_instr(list, TAC_STRI2INT, tmp, s, i);
        add_instr(list, TAC_JUMP, op_none(), op_none(), op_label(l_end));
        add_instr(list, TAC_LABEL, op_label(l_neg), op_none(), op_none());
        add_instr(list, TAC_MOVE, tmp, op_int(-1), op_none());
        add_instr(list, TAC_LABEL, op_label(l_end), op_none(), op_none());
        *res = tmp;
        return true;

    }
    if(strcmp(name, "subStr") == 0)
    {
        Operand s = tac_gen_exp(args->list[0], list);
        Operand start = tac_gen_exp(args->list[1], list);
        Operand end = tac_gen_exp(args->list[2], list);
        int l_err = new_label();
        int l_loop = new_label();
        int l_done = new_label();
        int l_end = new_label();

        Operand len = print_un(list,TAC_STRLEN,s);

        Operand c1 = print_bin(list,TAC_LT,start,op_int(0));
        add_instr(list, TAC_JUMP_IF_TRUE, op_none(), c1, op_label(l_err));
        Operand c2 = print_bin(list, TAC_LT, end, op_int(0));
        add_instr(list, TAC_JUMP_IF_TRUE, op_none(), c2, op_label(l_err));
        Operand c3 = print_bin(list, TAC_GT, start, end);
        add_instr(list, TAC_JUMP_IF_TRUE, op_none(), c3, op_label(l_err));
        Operand c4 = print_bin(list, TAC_LT, start, len);  
        add_instr(list, TAC_JUMP_IF_FALSE, op_none(), c4, op_label(l_err)); 
        Operand c5 = print_bin(list, TAC_GT, end, len);
        add_instr(list, TAC_JUMP_IF_TRUE, op_none(), c5, op_label(l_err)); 
        
        Operand tmp = op_temp(new_temp());
        Operand i = op_temp(new_temp());
        add_instr(list, TAC_MOVE, tmp, op_string(""), op_none());
        add_instr(list, TAC_MOVE, i, start, op_none());

        add_instr(list, TAC_LABEL, op_label(l_loop), op_none(), op_none());
        Operand cond = print_bin(list, TAC_LT, i, end); 
        add_instr(list, TAC_JUMP_IF_FALSE, op_none(), cond, op_label(l_done));
        Operand ch = print_bin(list, TAC_GETCHAR, s, i);
        add_instr(list, TAC_CONCAT, tmp, tmp, ch);
        add_instr(list, TAC_ADD, i, i, op_int(1));
        add_instr(list, TAC_JUMP, op_none(), op_none(), op_label(l_loop));

        add_instr(list, TAC_LABEL, op_label(l_done), op_none(), op_none());
        add_instr(list, TAC_JUMP, op_none(), op_none(), op_label(l_end));
        add_instr(list, TAC_LABEL, op_label(l_err), op_none(), op_none());
        add_instr(list, TAC_EXIT, op_none(), op_int(40), op_none());
        add_instr(list, TAC_LABEL, op_label(l_end), op_none(), op_none());
        *res = tmp;
        return true;

    }
    return false;
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
    unique_counter = 1;
    g_defs = NULL; 
    current_ret_type = NULL;     
    scope_reset();
}

