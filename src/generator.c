#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "generator.h"
#include "error.h"
#include <ctype.h>

static bool *temp_seen = NULL;          
static int temp_seen_cap = 0;
static int param_idx = 0;

static const char **var_seen = NULL;
static int var_seen_cnt = 0;
static int var_seen_cap = 0;

static void defined_reset(void)
{
    free(temp_seen);
    free(var_seen);

    temp_seen = NULL;
    var_seen = NULL;

    temp_seen_cap = 0;
    var_seen_cnt = 0;
    var_seen_cap = 0;
}

static bool is_param_slot(const char *name)
{
    return name[0] == '%' && isdigit((unsigned char)name[1]);
}

static bool first_time_temp(int id)
{
    if (id >= temp_seen_cap)
    {
        int new_cap = temp_seen_cap == 0 ? 128 : temp_seen_cap;
        while (new_cap <= id)
            new_cap *= 2;

        bool *p = realloc(temp_seen, new_cap * sizeof(bool));
        if (p == NULL)
            error_exit(ERR_INTERNAL, 0, "Memory allocation error in first_time_temp");
        for (int i = temp_seen_cap; i < new_cap; i++)
            p[i] = false;
        temp_seen = p;
        temp_seen_cap = new_cap;
    }
    if (temp_seen[id])
        return false;
    temp_seen[id] = true;
    return true;
}

static bool first_time_var(const char *name)
{
    for (int i = 0; i < var_seen_cnt; i++)
        if (strcmp(var_seen[i], name) == 0)
            return false;

    if (var_seen_cnt == var_seen_cap)
    {
        int new_cap = var_seen_cap == 0 ? 32 : var_seen_cap * 2;
        const char **p = realloc(var_seen, new_cap * sizeof(char *));
        if (p == NULL)
            error_exit(ERR_INTERNAL, 0, "Memory allocation error in first_time_var");
        var_seen = p;
        var_seen_cap = new_cap;
    }
    var_seen[var_seen_cnt++] = name;
    return true;
}

static void define_operand(Operand op, FILE *out)
{
    if(op.kind == OP_TEMP && first_time_temp(op.data.temp_id))
    {
        fprintf(out,"DEFVAR LF@%%T%d\n", op.data.temp_id);
    }
    else if (op.kind == OP_VAR && !is_param_slot(op.data.name) && first_time_var(op.data.name))
        fprintf(out, "DEFVAR LF@%s\n", op.data.name);
}

static bool is_function_label(const TACInstr *in)
{
    return in->op == TAC_LABEL && in->res.kind == OP_VAR;
}

static void print_defvars(const TacList *list, int from, int to, FILE *out)
{
    defined_reset();
    for (int i = from; i < to; i++)
    {
        const TACInstr *in = &list->items[i];
        if (in->op == TAC_LABEL)         
            continue;

        define_operand(in->res, out);
        if (in->op != TAC_CALL)          
            define_operand(in->a, out);
        define_operand(in->b, out);
    }
}

static int function_end(const TacList *list, int start)
{
    for (int j = start; j < list->count; j++)
        if (list->items[j].op == TAC_RETURN)
            return j;
    return list->count;
}

static void print_operand(Operand op, FILE *out)
{
    switch (op.kind)
    {
    case OP_INT:
        fprintf(out, "int@%d",op.data.int_val);
        break;
    case OP_DOUBLE:
        fprintf(out,"float@%a",op.data.double_val);
        break;

    case OP_STRING:
        fprintf(out, "string@");
        for (const unsigned char *p = (const unsigned char *)op.data.str_val; *p; p++)
        {
            if (*p <= 32 || *p == '#' || *p == '\\')
                fprintf(out, "\\%03d", *p);   
            else
                fputc(*p, out);
        }
        break;
    case OP_TEMP:
        fprintf(out,"LF@%%T%d",op.data.temp_id);
        break;

    case OP_VAR:
        fprintf(out,"LF@%s",op.data.name);
        break;

    case OP_LABEL:
        fprintf(out,"L%d", op.data.label_id);
        break;
    case OP_BOOL:
            fprintf(out, "bool@%s", op.data.bool_val ? "true" : "false");
            break;
    case OP_NIL:
        fprintf(out, "nil@nil");
        break;
    case OP_NONE:
        break;
    default:
        break;
    }
}

void generate_ifjcode26(TacList *tac_list, FILE *output)
{
    param_idx = 0;
    fprintf(output,".IFJcode26\n");
    fprintf(output,"CREATEFRAME\n");
    fprintf(output,"PUSHFRAME\n");
    int first_fn = tac_list->count;
    for (int i = 0; i < tac_list->count; i++)
    if (is_function_label(&tac_list->items[i])) { first_fn = i; break; }
    print_defvars(tac_list, 0, first_fn, output);

    for(int i = 0; i < tac_list->count; i++)
    {
        TACInstr *instr = &tac_list->items[i];
        switch (instr->op)
        {
        case TAC_LABEL:
            if (instr->res.kind == OP_VAR)
            {
                fprintf(output, "LABEL $%s\n", instr->res.data.name);
                print_defvars(tac_list, i + 1, function_end(tac_list, i + 1), output);
            }
            else
            {
                fprintf(output, "LABEL L%d\n", instr->res.data.label_id);
            }
            break;
        case TAC_MOVE:
            fprintf(output,"MOVE ");
            print_operand(instr->res,output);
            fprintf(output," ");
            print_operand(instr->a,output);
            fprintf(output,"\n");
            break;
        case TAC_ADD:
            fprintf(output,"ADD ");
            print_operand(instr->res,output);
            fprintf(output," ");
            print_operand(instr->a,output);
            fprintf(output," ");
            print_operand(instr->b,output);
            fprintf(output,"\n");
            break;
        case TAC_SUB:
            fprintf(output,"SUB ");
            print_operand(instr->res,output);
            fprintf(output," ");
            print_operand(instr->a,output);
            fprintf(output," ");
            print_operand(instr->b,output);
            fprintf(output,"\n");
            break;
        case TAC_DIV:
            fprintf(output,"DIV ");
            print_operand(instr->res,output);
            fprintf(output," ");
            print_operand(instr->a,output);
            fprintf(output," ");
            print_operand(instr->b,output);
            fprintf(output,"\n");
            break;
        case TAC_IDIV:
            fprintf(output,"IDIV ");
            print_operand(instr->res,output);
            fprintf(output," ");
            print_operand(instr->a,output);
            fprintf(output," ");
            print_operand(instr->b,output);
            fprintf(output,"\n");
            break;
        case TAC_MUL:
            fprintf(output,"MUL ");
            print_operand(instr->res,output);
            fprintf(output," ");
            print_operand(instr->a,output);
            fprintf(output," ");
            print_operand(instr->b,output);
            fprintf(output,"\n");
            break;
        case TAC_LT:
            fprintf(output,"LT ");
            print_operand(instr->res,output);
            fprintf(output," ");
            print_operand(instr->a,output);
            fprintf(output," ");
            print_operand(instr->b,output);
            fprintf(output,"\n");
            break;
        case TAC_GT:
            fprintf(output,"GT ");
            print_operand(instr->res,output);
            fprintf(output," ");
            print_operand(instr->a,output);
            fprintf(output," ");
            print_operand(instr->b,output);
            fprintf(output,"\n");
            break;
        case TAC_EQ:
            fprintf(output,"EQ ");
            print_operand(instr->res,output);
            fprintf(output," ");
            print_operand(instr->a,output);
            fprintf(output," ");
            print_operand(instr->b,output);
            fprintf(output,"\n");
            break;
        case TAC_JUMP:
            fprintf(output,"JUMP ");
            print_operand(instr->b,output);
            fprintf(output,"\n");
            break;
        case TAC_JUMP_IF_FALSE:
            fprintf(output, "JUMPIFEQ ");
            print_operand(instr->b, output); 
            fprintf(output, " ");
            print_operand(instr->a, output); 
            fprintf(output, " bool@false\n"); 
            break;
        case TAC_JUMP_IF_TRUE:
            fprintf(output, "JUMPIFEQ ");
            print_operand(instr->b, output); 
            fprintf(output, " ");
            print_operand(instr->a, output); 
            fprintf(output, " bool@true\n");  
            break;
        case TAC_NOT:
            fprintf(output, "NOT ");
            print_operand(instr->res, output);
            fprintf(output, " ");
            print_operand(instr->a, output);
            fprintf(output, "\n");
            break;
        case TAC_RETURN:
            fprintf(output, "RETURN\n");
            break;
        case TAC_PARAM:
            if (param_idx == 0)
            {
                fprintf(output, "CREATEFRAME\n");
            }
            param_idx++;
            fprintf(output, "DEFVAR TF@%%%d\n", param_idx);
            fprintf(output, "MOVE TF@%%%d ", param_idx);
            print_operand(instr->res, output);
            fprintf(output, "\n");
            break;
        case TAC_CALL:
            if (param_idx == 0)
            {
                fprintf(output, "CREATEFRAME\n");
            }
            fprintf(output, "PUSHFRAME\n");
            fprintf(output, "CALL $%s\n", instr->a.data.name);
            fprintf(output, "POPFRAME\n");
            if (instr->res.kind != OP_NONE)
            {
                fprintf(output, "MOVE ");
                print_operand(instr->res, output);
                fprintf(output, " TF@%%retval\n");
            }
            param_idx = 0;
            break;
        case TAC_WRITE:
            fprintf(output, "WRITE ");
            print_operand(instr->a, output);
            fprintf(output, "\n");
            break;
        case TAC_INT2FLOAT:
            fprintf(output, "INT2FLOAT ");
            print_operand(instr->res, output);
            fprintf(output, " ");
            print_operand(instr->a, output);
            fprintf(output, "\n");
            break;
        case TAC_CONCAT:
            fprintf(output,"CONCAT ");
            print_operand(instr->res,output);
            fprintf(output," ");
            print_operand(instr->a,output);
            fprintf(output," ");
            print_operand(instr->b,output);
            fprintf(output,"\n");
            break;
        default:
            break;
        }
    }
    fprintf(output,"EXIT int@0\n");
    defined_reset();
}