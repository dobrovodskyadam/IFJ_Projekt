// @file ast.c
// @brief
// @date 12-11-2026
//
// @author Lukas Sofka (xsofkal00)
// @author Email: xsofkal00@stud.fit.vutbr.cz
// Faculty of Information Technology, Brno University of Technology

#include "ast.h"
#include "error.h"
#include <stdlib.h>

Node *ASTNewNode(NodeKind kind, unsigned int line) {
    Node *node = calloc(1, sizeof(Node));
    if (!node) {
        error_exit(ERR_INTERNAL, line, "Memory allocation error in ASTNewNode");
    }

    node->kind = kind;
    node->line = line;
    node->type.kind = TYPE_UNKNOWN;

    return node;
};

void ASTFree(Node *ptr) {
    if (ptr == NULL) {
        return;
    }

    switch (ptr->kind) {

    case NODE_PROGRAM:
        NodeListFree(&ptr->data.program_data.definitions);
        break;

    case NODE_FUNCTION_DEF:
        free(ptr->data.function_def_data.name);
        NodeListFree(&ptr->data.function_def_data.params);
        ASTFree(ptr->data.function_def_data.body);
        break;

    case NODE_PARAMETER:
        free(ptr->data.param_data.name);
        break;

    case NODE_BLOCK:
        NodeListFree(&ptr->data.block_data.statements);
        break;

    case NODE_VARIABLE_DECLARATION:
        free(ptr->data.var_decl_data.name);
        ASTFree(ptr->data.var_decl_data.value);
        break;

    case NODE_ASSIGN:
        free(ptr->data.assign_data.name);
        ASTFree(ptr->data.assign_data.value);
        break;

    case NODE_IF_STATEMENT:
    case NODE_IF_VALUE:
        ASTFree(ptr->data.if_data.condition);
        ASTFree(ptr->data.if_data.then_part);
        ASTFree(ptr->data.if_data.else_part);
        break;

    case NODE_WHILE_CYCLE:
        ASTFree(ptr->data.while_data.condition);
        ASTFree(ptr->data.while_data.body);
        break;

    case NODE_FOR_CYCLE:
        free(ptr->data.for_data.name);
        ASTFree(ptr->data.for_data.range);
        NodeListFree(&ptr->data.for_data.guards);
        ASTFree(ptr->data.for_data.body);
        break;

    case NODE_RETURN:
        ASTFree(ptr->data.return_data.value);
        break;

    case NODE_BINARY_OPERATOR:
        ASTFree(ptr->data.bin_op_data.left);
        ASTFree(ptr->data.bin_op_data.right);
        break;

    case NODE_UNARY_OPERATOR:
        ASTFree(ptr->data.un_op_data.operand);
        break;

    case NODE_FUNCTION_CALL:
        free(ptr->data.call_data.name);
        NodeListFree(&ptr->data.call_data.arguments);
        break;

    case NODE_LIST_LITERAL:
        NodeListFree(&ptr->data.list_data.items);
        break;

    case NODE_EXPRESSION:
        ASTFree(ptr->data.expression_data);
        break;

    case NODE_ID:
        free(ptr->data.variable_name);
        break;

    case NODE_STRING:
        free(ptr->data.string_val);
        break;

    default:
        break;
    }

    free(ptr);
}

void NodeListInit(NodeList *ptr) {
    ptr->list = NULL;
    ptr->capacity = 0;
    ptr->count = 0;
}

void NodeListFree(NodeList *ptr) {
    for (unsigned int i = 0; i < ptr->count; i++) {
        ASTFree(ptr->list[i]);
    }
    free(ptr->list);
}

void NodeListAppend(NodeList *ptr, Node *node) {
    if (ptr->capacity == ptr->count) {
        unsigned int new_capacity = ptr->capacity == 0 ? INITIAL_LIST_CAPACITY : ptr->capacity * 2;
        if (new_capacity < ptr->capacity) {
            error_exit(ERR_INTERNAL, node->line, "Int overflow in NodeListAppend");
        }

        Node **new_list = realloc(ptr->list, sizeof(Node *) * new_capacity);
        if (!new_list) {
            error_exit(ERR_INTERNAL, node->line, "Memory allocation error in NodeListAppend");
        }

        ptr->list = new_list;
        ptr->capacity = new_capacity;
    }

    ptr->list[ptr->count] = node;
    ptr->count++;
}

Type *TypeInit(int line) {
    Type *type = malloc(sizeof(Type));
    if (!type) {
        error_exit(ERR_INTERNAL, line, "Memory allocation error in TypeInit");
    }

    return type;
}
