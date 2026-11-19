#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "generator.h"
#include "error.h"

static bool *tem_defined = NULL;
static int temp_defined_capacity = 0;

void temp_defined(int temp_id, FILE *out)
{
    int new_capacity;
    if(temp_id >= temp_defined_capacity)
    {
        new_capacity = temp_defined_capacity == 0 ? 128 : temp_defined_capacity * 2;

        while (new_capacity <= temp_id) {
            new_capacity *= 2;
        }

        bool *p = realloc(tem_defined, new_capacity * sizeof(bool));
        if( p == NULL)
        {
            error_exit(ERR_INTERNAL, 0, "Memory allocation error in temp_defined");
        }

        for (int i = temp_defined_capacity; i < new_capacity; i++) {
            p[i] = false;
        }

        tem_defined = p;
        temp_defined_capacity = new_capacity;
    }

    if(!tem_defined[temp_id])
    {
        fprintf(out, "DEFVAR LF@T%d\n", temp_id);
        tem_defined[temp_id] = true;
    }

}

void free_temp_defined(void) {
    if (temp_defined != NULL) {
        free(tem_defined);
        tem_defined = NULL;
        temp_defined_capacity = 0;
    }
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
        fprintf(out,"string@%s",op.data.str_val);
        break;

    case OP_TEMP:
        fprintf(out,"LF@T%d",op.data.temp_id);
        break;

    case OP_VAR:
        fprintf(out,"LF@%s",op.data.name);
        break;

    case OP_LABEL:
        fprintf(out,"L%d", op.data.label_id);
        break;

    case OP_NONE:
        break;
    default:
        break;
    }
}

void generate_ifjcode26(TacList *tac_list, FILE *output)
{
    fprintf(output,".IFJcode26\n");
    free_temp_defined();

    for(int i = 0; i < tac_list->count; i++)
    {
        TACInstr *instr = &tac_list->items[i];

        if(instr->res.kind == OP_TEMP)
            temp_defined(instr->res.data.temp_id,output);

        switch (instr->op)
        {
        case TAC_LABEL:
            fprintf(output,"LABEL ");
            print_operand(instr->res,output);
            fprintf(output,"\n");
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
            fprintf(output, "JUMPIFNEQ ");
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
        default:
            break;
        }
    }

}