// @file ast.c
// @brief
// @date 12-11-2026
//
// @author Lukas Sofka (xsofkal00)
// @author Email: xsofkal00@stud.fit.vutbr.cz
// Faculty of Information Technology, Brno University of Technology

#include "ast.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>

// @brief
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

// @brief
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

// @brief
void NodeListInit(NodeList *ptr) {
    ptr->list = NULL;
    ptr->capacity = 0;
    ptr->count = 0;
}

// @brief
void NodeListFree(NodeList *ptr) {
    for (unsigned int i = 0; i < ptr->count; i++) {
        ASTFree(ptr->list[i]);
    }
    free(ptr->list);
}

// @brief
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

// @brief
Type *TypeInit(int line) {
    Type *type = malloc(sizeof(Type));
    if (!type) {
        error_exit(ERR_INTERNAL, line, "Memory allocation error in TypeInit");
    }

    return type;
}

// clang-format off
// @brief
static const char *kind_names[] = {
    [NODE_PROGRAM]              = "PROGRAM",
    [NODE_FUNCTION_DEF]         = "FUNCTION_DEF",
    [NODE_PARAMETER]            = "PARAMETER",
    [NODE_BLOCK]                = "BLOCK",
    [NODE_VARIABLE_DECLARATION] = "VARIABLE_DECLARATION",
    [NODE_ASSIGN]               = "ASSIGN",
    [NODE_IF_STATEMENT]         = "IF_STATEMENT",
    [NODE_FOR_CYCLE]            = "FOR_CYCLE",
    [NODE_WHILE_CYCLE]          = "WHILE_CYCLE",
    [NODE_RETURN]               = "RETURN",
    [NODE_BINARY_OPERATOR]      = "BINARY_OPERATOR",
    [NODE_UNARY_OPERATOR]       = "UNARY_OPERATOR",
    [NODE_IF_VALUE]             = "IF_VALUE",
    [NODE_LIST_LITERAL]         = "LIST_LITERAL",
    [NODE_EXPRESSION]           = "EXPRESSION",
    [NODE_FUNCTION_CALL]        = "FUNCTION_CALL",
    [NODE_ID]                   = "ID",
    [NODE_INT]                  = "INT",
    [NODE_DOUBLE]               = "DOUBLE",
    [NODE_BOOLEAN]              = "BOOLEAN",
    [NODE_STRING]               = "STRING",
    [NODE_UNIT]                 = "UNIT",
};
// clang-format on

// @brief Helper function for printing NodeKind
const char *NodeKindToString(NodeKind kind) {
    return kind_names[kind];
}

// @brief Helper function for printing Type
void TypePrint(const Type *type) {
    switch (type->kind) {
    case TYPE_UNKNOWN:
        fprintf(stderr, "Unknown");
        break;

    case TYPE_INT:
        fprintf(stderr, "Int");
        break;

    case TYPE_DOUBLE:
        fprintf(stderr, "Double");
        break;

    case TYPE_STRING:
        fprintf(stderr, "String");
        break;

    case TYPE_BOOL:
        fprintf(stderr, "Boolean");
        break;

    case TYPE_UNIT:
        fprintf(stderr, "Unit");
        break;

    case TYPE_LIST:
        fprintf(stderr, "List[");
        TypePrint(type->elem);
        fprintf(stderr, "]");
        break;
    }
}

// @brief
void PrintIndent(int depth) {
    for (int i = 0; i < depth; i++) {
        fprintf(stderr, "  ");
    }
}

// @brief
static void NodeListPrint(const NodeList *list, int indent) {
    for (unsigned int i = 0; i < list->count; i++) {
        ASTprint(list->list[i], indent);
    }
}

// @brief
static const char *boolToString(bool value) {
    return value ? "True" : "False";
}

// @brief
void ASTprint(Node *root, int indent) {
    PrintIndent(indent);
    indent++;

    if (root == NULL) {
        fprintf(stderr, "NULL\n");
        return;
    }

    // Switch between all kinds of nodes
    switch (root->kind) {
    case NODE_PROGRAM:
        // Print program
        // (e.g. Program (line 4) definitions=4)
        fprintf(stderr, "Program (line %u) definitions=%u\n", root->line,
                root->data.program_data.definitions.count);
        NodeListPrint(&root->data.program_data.definitions, indent);
        break;

    case NODE_FUNCTION_DEF:
        // Print function declaration
        // (e.g. Function def (line 10) name=foo main=False param_count=2 return=Int)
        fprintf(stderr, "Function def (line %u) name=%s main=%s param_count=%i return=", root->line,
                root->data.function_def_data.name,
                boolToString(root->data.function_def_data.is_main),
                root->data.function_def_data.params.count);
        TypePrint(&root->data.function_def_data.return_type);

        // Prints parameters of the function declaration
        fprintf(stderr, "\n");
        NodeListPrint(&root->data.function_def_data.params, indent);

        // Prints body
        PrintIndent(indent);
        fprintf(stderr, "Body:\n");
        ASTprint(root->data.function_def_data.body, indent + 1);

        break;

    case NODE_PARAMETER:
        // Print parameter
        // (e.g. Parameter (line ...) name=n declared=Int)
        fprintf(stderr, "Parameter (line %u) name=%s declared=", root->line,
                root->data.param_data.name);
        TypePrint(&root->data.param_data.type);
        fprintf(stderr, "\n");
        break;

    case NODE_BLOCK:
        // Print block
        // (e.g. Block (line ...) statements=1))
        fprintf(stderr, "Block (line %u) statements=%u\n", root->line,
                root->data.block_data.statements.count);

        // Print all statements
        NodeListPrint(&root->data.block_data.statements, indent);
        break;

    case NODE_VARIABLE_DECLARATION:
        // Print variable declaration
        // (e.g. Variable declaration (line ...) name=k val=False declared=Unknown)

        fprintf(stderr, "Variable declaration (line %u) name=%s val=%s declared=", root->line,
                root->data.var_decl_data.name, boolToString(root->data.var_decl_data.is_val));
        TypePrint(&root->data.var_decl_data.type);
        fprintf(stderr, "\n");

        // Print the value block
        PrintIndent(indent);
        fprintf(stderr, "Value:\n");
        ASTprint(root->data.var_decl_data.value, indent + 1);

        break;

    case NODE_ASSIGN:
        // Print the assign
        // (e.g. Assign (line ...) name=i)
        fprintf(stderr, "Assign (line %u) name=%s\n", root->line, root->data.assign_data.name);

        // Print the value block
        PrintIndent(indent);
        fprintf(stderr, "Value:\n");
        ASTprint(root->data.assign_data.value, indent + 1);
        break;

    case NODE_IF_STATEMENT:
    case NODE_IF_VALUE:
        // TODO FILL THE REST
        fprintf(stderr, "%s (line %u)\n", root->kind == NODE_IF_VALUE ? "If value" : "If statement",
                root->line);

        PrintIndent(indent);
        fprintf(stderr, "Condition:\n");
        ASTprint(root->data.if_data.condition, indent + 1);
        PrintIndent(indent);
        fprintf(stderr, "Then:\n");
        ASTprint(root->data.if_data.then_part, indent + 1);
        PrintIndent(indent);
        fprintf(stderr, "Else:\n");
        ASTprint(root->data.if_data.else_part, indent + 1);
        break;

    case NODE_FOR_CYCLE:
        fprintf(stderr, "For cycle (line %u) variable=%s guards=%u\n", root->line,
                root->data.for_data.name, root->data.for_data.guards.count);

        PrintIndent(indent);
        fprintf(stderr, "Range:\n");
        ASTprint(root->data.for_data.range, indent + 1);
        for (unsigned int i = 0; i < root->data.for_data.guards.count; i++) {
            PrintIndent(indent);
            fprintf(stderr, "Guard:\n");
            ASTprint(root->data.for_data.guards.list[i], indent + 1);
        }
        PrintIndent(indent);
        fprintf(stderr, "Body:\n");
        ASTprint(root->data.for_data.body, indent + 1);
        break;

    case NODE_WHILE_CYCLE:
        fprintf(stderr, "While cycle (line %u)\n", root->line);
        PrintIndent(indent);
        fprintf(stderr, "Condition:\n");
        ASTprint(root->data.while_data.condition, indent + 1);
        PrintIndent(indent);
        fprintf(stderr, "Body:\n");
        ASTprint(root->data.while_data.body, indent + 1);
        break;

    case NODE_RETURN:
        fprintf(stderr, "Return (line %u)\n", root->line);
        PrintIndent(indent);
        fprintf(stderr, "Value:\n");
        ASTprint(root->data.return_data.value, indent + 1);
        break;

    case NODE_BINARY_OPERATOR:
        fprintf(stderr, "Binary operator (line %u) op=%s\n", root->line,
                tokenTypeName(root->data.bin_op_data.op));
        PrintIndent(indent);
        fprintf(stderr, "Left:\n");
        ASTprint(root->data.bin_op_data.left, indent + 1);
        PrintIndent(indent);
        fprintf(stderr, "Right:\n");
        ASTprint(root->data.bin_op_data.right, indent + 1);
        break;

    case NODE_UNARY_OPERATOR:
        fprintf(stderr, "Unary operator (line %u) op=%s\n", root->line,
                tokenTypeName(root->data.un_op_data.op));
        PrintIndent(indent);
        fprintf(stderr, "Operand:\n");
        ASTprint(root->data.un_op_data.operand, indent + 1);
        break;

    case NODE_LIST_LITERAL:
        fprintf(stderr, "List literal (line %u) items=%u\n", root->line,
                root->data.list_data.items.count);
        NodeListPrint(&root->data.list_data.items, indent);
        break;

    case NODE_EXPRESSION:
        fprintf(stderr, "Expression (line %u)\n", root->line);
        ASTprint(root->data.expression_data, indent);
        break;

    case NODE_FUNCTION_CALL:
        fprintf(stderr, "Function call (line %u) name=%s arguments=%u\n", root->line,
                root->data.call_data.name, root->data.call_data.arguments.count);
        NodeListPrint(&root->data.call_data.arguments, indent);
        break;

    case NODE_ID:
        fprintf(stderr, "ID (line %u) name=%s\n", root->line, root->data.variable_name);
        break;

    case NODE_INT:
        fprintf(stderr, "Int (line %u) value=%d\n", root->line, root->data.int_value);
        break;

    case NODE_DOUBLE:
        fprintf(stderr, "Double (line %u) value=%g\n", root->line, root->data.double_value);
        break;

    case NODE_BOOLEAN:
        fprintf(stderr, "Boolean (line %u) value=%s\n", root->line,
                boolToString(root->data.bool_value));
        break;

    case NODE_STRING:
        fprintf(stderr, "String (line %u) value=\"%s\"\n", root->line, root->data.string_val);
        break;

    case NODE_UNIT:
        fprintf(stderr, "Unit (line %u)\n", root->line);
        break;
    }
}
