/// @file semantic.c
/// @brief
/// @date 12-11-2026
///
/// @author Lukas Sofka (xsofkal00)
/// @author Email: xsofkal00@stud.fit.vutbr.cz
/// Faculty of Information Technology, Brno University of Technology

#include "semantic.h"
#include "ast.h"
#include "error.h"
#include "expression_parser.h"
#include "parser.h"
#include "scanner.h"
#include "symtable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO
// Implicit Int -> Float
// Last statement of function -> return value
// List details
// If value

static Node *current_function;

typedef struct {
    const char *name;
    DataType return_type;
    unsigned int param_count;
    DataType *params;
} BuiltIn;

// clang-format off

/// @brief Built-in functions signatures
/// Used for checking semantic correctness
const BuiltIn built_in_functions[] =
    {
        {"print",       TYPE_UNIT,   1, (DataType[]){TYPE_ANY}},
        {"readLine",    TYPE_STRING, 0, NULL},
        {"readInt",     TYPE_INT,    0, NULL},
        {"readDouble",  TYPE_DOUBLE, 0, NULL},
        {"readBoolean", TYPE_BOOL,   0, NULL},
        {"toStr",       TYPE_STRING, 1, (DataType[]){TYPE_ANY}},
        {"toInt",       TYPE_INT,    1, (DataType[]){TYPE_DOUBLE}},
        {"strLength",   TYPE_INT,    1, (DataType[]){TYPE_STRING}},
        {"subStr",      TYPE_STRING, 3, (DataType[]){TYPE_STRING, TYPE_INT, TYPE_INT}},
        {"ord",         TYPE_INT,    2, (DataType[]){TYPE_STRING, TYPE_INT}},
        {"chr",         TYPE_STRING, 1, (DataType[]){TYPE_INT}},

};
// clang-format on

/// @brief
Type declaration_to_type(Node *node) {
    switch (node->kind) {
    case NODE_VARIABLE_DECLARATION:
        return node->data.var_decl_data.type;

    case NODE_PARAMETER:
        return node->data.param_data.type;

    case NODE_FOR_CYCLE:
        return node->data.for_data.var_type;

    default: {
        Type temp = {TYPE_UNKNOWN, NULL};
        return temp;
    }
    }
}

/// @brief
bool type_eq(Type a, Type b) {
    if (a.kind != b.kind) {
        return false;
    }
    if (a.kind == TYPE_LIST) {
        if (a.elem->kind == TYPE_UNKNOWN || b.elem->kind == TYPE_UNKNOWN) {
            return true;
        }

        return type_eq(*a.elem, *b.elem);
    }

    return true;
}

/// @brief
char *copy_string(const char *str) {
    char *copy = malloc(strlen(str) + 1);
    if (!copy) {
        error_exit(ERR_INTERNAL, 0, "memory allocation error in copy_string");
    }

    strcpy(copy, str);
    return copy;
}

/// @brief Checks whether the type is Int or Double
bool is_numeric(Type item) {
    return (item.kind == TYPE_INT || item.kind == TYPE_DOUBLE);
}

/// @brief
Type numeric_result(Type a, Type b) {
    Type result = {TYPE_INT, NULL};

    if (a.kind == TYPE_DOUBLE || b.kind == TYPE_DOUBLE) {
        result.kind = TYPE_DOUBLE;
    }
    return result;
}

/// TODO
Type result_type(symtab_t *symtable, Node *node) {
    Type temp = {TYPE_UNKNOWN, NULL};
    return temp;
}

/// @brief
void built_in_function(symtab_t *symtable, NodeList *built_in_ast) {
    unsigned int count = sizeof(built_in_functions) / sizeof(built_in_functions[0]);

    for (unsigned int i = 0; i < count; i++) {
        Node *function = ASTNewNode(NODE_FUNCTION_DEF, 0);
        function->data.function_def_data.return_type.kind = built_in_functions[i].return_type;
        function->data.function_def_data.name = copy_string(built_in_functions[i].name);

        for (unsigned int j = 0; j < built_in_functions[i].param_count; j++) {
            Node *parameter = ASTNewNode(NODE_PARAMETER, 0);
            parameter->data.param_data.type.kind = built_in_functions[i].params[j];
            NodeListAppend(&function->data.function_def_data.params, parameter);
        }

        item_add(symtable, function->data.function_def_data.name, function);
        NodeListAppend(built_in_ast, function);
    }
}

/// @brief
void collect_functions(symtab_t *symtable, Node *root) {
    NodeList *function_definitions = &root->data.program_data.definitions;
    for (unsigned int i = 0; i < function_definitions->count; i++) {
        Node *definition = function_definitions->list[i];
        if (!item_add(symtable, definition->data.function_def_data.name, definition)) {
            error_exit(ERR_SEMANTIC_REDEFINITION, definition->line,
                       "redefinitions of a function %s", definition->data.function_def_data.name);
        }
    }
}

/// @brief
void check_main(Node *root) {
    NodeList *function_definitions = &root->data.program_data.definitions;
    Node *main_definition = NULL;
    int main_count = 0;

    for (unsigned int i = 0; i < function_definitions->count; i++) {
        if (function_definitions->list[i]->data.function_def_data.is_main) {
            main_definition = function_definitions->list[i];
            main_count++;
        }
    }

    if (main_count == 0) {
        error_exit(ERR_SEMANTIC_UNDEFINED, 0, "missing main function (add @main)");
    } else if (main_count > 1) {
        error_exit(ERR_SEMANTIC_REDEFINITION, main_definition->line,
                   "redefinition of main function");
    }

    if (main_definition->data.function_def_data.params.count != 0) {
        error_exit(ERR_SEMANTIC_ARGUMENTS, main_definition->line,
                   "main function must not have parameters (remove arguments from main function)");
    }

    DataType return_kind = main_definition->data.function_def_data.return_type.kind;
    if (return_kind != TYPE_UNIT) {
        error_exit(ERR_SEMANTIC_ARGUMENTS, main_definition->line, "main function must return Unit");
    }
}

/// @brief
Type check_expression(symtab_t *symtable, Node *node) {
    Type itemType = {TYPE_UNKNOWN, NULL};
    if (!node) {
        return itemType;
    }

    switch (node->kind) {
    case NODE_INT:
        itemType.kind = TYPE_INT;
        break;

    case NODE_DOUBLE:
        itemType.kind = TYPE_DOUBLE;
        break;

    case NODE_BOOLEAN:
        itemType.kind = TYPE_BOOL;
        break;

    case NODE_STRING:
        itemType.kind = TYPE_STRING;
        break;

    case NODE_UNIT:
        itemType.kind = TYPE_UNIT;
        break;

    // Checks correctness of list literal (e.g. List(1, 2, 3, a))
    case NODE_LIST_LITERAL: {
        NodeList *items = &node->data.list_data.items;
        Type element_type = {TYPE_UNKNOWN, NULL};

        for (unsigned int i = 0; i < items->count; i++) {
            Type item_type = check_expression(symtable, items->list[i]);

            if (element_type.kind == TYPE_UNKNOWN) {
                element_type = item_type;
            } else if (item_type.kind != TYPE_UNKNOWN && !type_eq(element_type, item_type)) {
                error_exit(ERR_SEMANTIC_TYPE, items->list[i]->line,
                           "declaration of list with mismatching types, expected %s, got %s",
                           DataTypeToString(element_type.kind), DataTypeToString(item_type.kind));
            }
        }

        itemType.kind = TYPE_LIST;
        itemType.elem = TypeInit(node->line);
        *itemType.elem = element_type;
        break;
    }

    case NODE_ID: {
        item_t item = get_item(symtable, node->data.variable_name);
        // Check whether the variable exists
        if (item.scope == -1) {
            error_exit(ERR_SEMANTIC_UNDEFINED, node->line, "undefined variable %s",
                       node->data.variable_name);
        }
        // Check whether functions was used as variable
        if (item.decl->kind == NODE_FUNCTION_DEF) {
            error_exit(ERR_SEMANTIC_TYPE, node->line, "function %s used as variable",
                       node->data.variable_name);
        }

        itemType = declaration_to_type(item.decl);
        break;
    }

    case NODE_BINARY_OPERATOR: {
        BinaryOpData *bin_data = &node->data.bin_op_data;

        if (bin_data->op == TOKEN_DOT) {
            // TODO
            break;
        }

        Type left = check_expression(symtable, bin_data->left);
        Type right = check_expression(symtable, bin_data->right);
        Type num_result = numeric_result(left, right);

        switch (bin_data->op) {
        case TOKEN_PLUS:
            if (is_numeric(left) && is_numeric(right)) {
                itemType = num_result;
            } else if (left.kind == TYPE_STRING && right.kind == TYPE_STRING) {
                itemType.kind = TYPE_STRING;
            } else {
                error_exit(ERR_SEMANTIC_TYPE, node->line,
                           "operands must be either both numeric or strings");
            }

            break;

        case TOKEN_MINUS:
        case TOKEN_MUL:
        case TOKEN_DIV:
            if (!is_numeric(left) || !is_numeric(right)) {
                error_exit(ERR_SEMANTIC_TYPE, node->line, "operands must be numeric");
            }

            itemType = num_result;
            break;

        case TOKEN_LS:
        case TOKEN_LS_EQ:
        case TOKEN_GR:
        case TOKEN_GR_EQ:
            if ((is_numeric(left) && is_numeric(right)) ||
                (left.kind == TYPE_STRING && right.kind == TYPE_STRING)) {
                itemType.kind = TYPE_BOOL;
            } else {
                error_exit(ERR_SEMANTIC_TYPE, node->line,
                           "operands must be either both numeric or strings");
            }
            break;

        case TOKEN_EQ:
        case TOKEN_NEQ:
            if ((is_numeric(left) && is_numeric(right)) || type_eq(left, right)) {
                itemType.kind = TYPE_BOOL;
            } else {
                error_exit(ERR_SEMANTIC_TYPE, node->line,
                           "operands must be numeric or of the same type");
            }
            break;

        case TOKEN_AND:
        case TOKEN_OR:
            if (left.kind == TYPE_BOOL && right.kind == TYPE_BOOL) {
                itemType.kind = TYPE_BOOL;
            } else {
                error_exit(ERR_SEMANTIC_TYPE, node->line, "operands must be boolean");
            }
            break;

        case TOKEN_CONCATENATION:
            if (!type_eq(left, right) || left.kind != TYPE_LIST) {
                error_exit(ERR_SEMANTIC_TYPE, node->line,
                           "both operands of concatenation must be list of same the same type");
            }

            itemType = left;
            break;

        case TOKEN_BY:
            if (left.kind != TYPE_RANGE || right.kind != TYPE_INT) {
                error_exit(ERR_SEMANTIC_TYPE, node->line,
                           "operands in by must be range and integer");
            } else if (bin_data->right->kind == NODE_INT && bin_data->right->data.int_value == 0) {
                error_exit(ERR_SEMANTIC_OTHER, node->line,
                           "zero step in for cycle is not permitted");
            }

            itemType.kind = TYPE_RANGE;
            break;

        case TOKEN_UNTIL:
        case TOKEN_TO:
            if (left.kind != TYPE_INT || right.kind != TYPE_INT) {
                error_exit(ERR_SEMANTIC_TYPE, node->line, "operands in range must be integers");
            }

            itemType.kind = TYPE_RANGE;
            break;

        default:
            break;
        }

        break;
    }

    case NODE_UNARY_OPERATOR: {
        Type type = check_expression(symtable, node->data.un_op_data.operand);
        unsigned int op = node->data.un_op_data.op;
        switch (op) {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            if (type.kind != TYPE_INT && type.kind != TYPE_DOUBLE) {
                error_exit(ERR_SEMANTIC_TYPE, node->line, "type mismatch in %s", tokenTypeName(op));
            }
            break;

        case TOKEN_NOT:
            if (type.kind != TYPE_BOOL) {
                error_exit(ERR_SEMANTIC_TYPE, node->line, "type mismatch in %s", tokenTypeName(op));
            }
            break;

        default:
            break;
        }

        itemType = type;
        break;
    }

    case NODE_FUNCTION_CALL: {
        item_t item = get_item(symtable, node->data.call_data.name);

        // Check whether the called function exists
        if (item.scope == -1) {
            error_exit(ERR_SEMANTIC_UNDEFINED, node->line, "undefined function");
        }
        // Check whether variable was used as a function
        else if (item.decl->kind != NODE_FUNCTION_DEF) {
            error_exit(ERR_SEMANTIC_REDEFINITION, node->line, "variable used as function call");
        }

        NodeList *args = &node->data.call_data.arguments;
        NodeList *params = &item.decl->data.function_def_data.params;

        // Check parameter count against the argument count
        if (args->count != params->count) {
            error_exit(ERR_SEMANTIC_ARGUMENTS, node->line, "wrong number of arguments in %s",
                       node->data.call_data.name);
        }

        // Check type match between all parameters and arguments
        for (unsigned int i = 0; i < args->count; i++) {
            Type arg_type = check_expression(symtable, args->list[i]);
            Type param_type = params->list[i]->data.param_data.type;

            if (!type_eq(arg_type, param_type) && param_type.kind != TYPE_ANY) {
                error_exit(ERR_SEMANTIC_ARGUMENTS, args->list[i]->line,
                           "argument %u of %s has a wrong type", i + 1, node->data.call_data.name);
            }
        }

        itemType = item.decl->data.function_def_data.return_type;
        break;
    }

    case NODE_IF_VALUE: {
        // TODO
        break;
    }

    default:
        break;
    }

    node->type = itemType;
    return itemType;
}

/// @brief
void check_statement(symtab_t *symtable, Node *node) {
    if (!node) {
        return;
    }

    switch (node->kind) {
    case NODE_BLOCK:
        scope_add(symtable);
        NodeList *statemenst = &node->data.block_data.statements;
        for (unsigned int i = 0; i < statemenst->count; i++) {
            check_statement(symtable, statemenst->list[i]);
        }
        scope_rm(symtable);
        break;

    case NODE_VARIABLE_DECLARATION: {
        Type value_type = check_expression(symtable, node->data.var_decl_data.value);
        Type *declared_type = &node->data.var_decl_data.type;

        if (declared_type->kind == TYPE_UNKNOWN) {
            *declared_type = value_type;
        } else if (!type_eq(*declared_type, value_type)) {
            error_exit(ERR_SEMANTIC_TYPE, node->line, "type mismatch in declaration of %s",
                       node->data.var_decl_data.name);
        }

        if (!item_add(symtable, node->data.var_decl_data.name, node)) {
            error_exit(ERR_SEMANTIC_REDEFINITION, node->line, "redefinition of a variable %s",
                       node->data.var_decl_data.name);
        }
        break;
    }

    case NODE_ASSIGN: {
        item_t item = get_item(symtable, node->data.assign_data.name);

        if (item.scope == -1) {
            error_exit(ERR_SEMANTIC_UNDEFINED, node->line, "undefined variable %s",
                       node->data.assign_data.name);
        }

        switch (item.decl->kind) {
        case NODE_VARIABLE_DECLARATION:
            if (item.decl->data.var_decl_data.is_val) {
                error_exit(ERR_SEMANTIC_IMMUTABLE, node->line, "assignment to value %s",
                           node->data.assign_data.name);
            }
            break;

        case NODE_PARAMETER:
            error_exit(ERR_SEMANTIC_IMMUTABLE, node->line, "parameter %s is immutable",
                       item.decl->data.param_data.name);
            break;

        case NODE_FOR_CYCLE:
            error_exit(ERR_SEMANTIC_IMMUTABLE, node->line, "variable %s in for cycle is immutable",
                       item.decl->data.for_data.name);
            break;

        case NODE_FUNCTION_DEF:
            error_exit(ERR_SEMANTIC_IMMUTABLE, node->line,
                       "cannot write into function identifier %s",
                       item.decl->data.function_def_data.name);
            break;

        default:
            break;
        }

        // Type checking for assign
        Type value_type = check_expression(symtable, node->data.assign_data.value);
        Type var_type = declaration_to_type(item.decl);
        if (!type_eq(var_type, value_type)) {
            error_exit(ERR_SEMANTIC_TYPE, node->line, "type mismatch in assignment to %s",
                       node->data.assign_data.name);
        }
        break;
    }

    case NODE_IF_STATEMENT: {
        Type condition = check_expression(symtable, node->data.if_data.condition);
        if (condition.kind != TYPE_BOOL) {
            error_exit(ERR_SEMANTIC_TYPE, node->line, "condition is not of type boolean");
        }

        check_statement(symtable, node->data.if_data.then_part);

        if (node->data.if_data.else_part) {
            check_statement(symtable, node->data.if_data.else_part);
        }

        break;
    }

    case NODE_WHILE_CYCLE: {
        Type condition = check_expression(symtable, node->data.while_data.condition);
        if (condition.kind != TYPE_BOOL) {
            error_exit(ERR_SEMANTIC_TYPE, node->line, "condition is not of type boolean");
        }

        check_statement(symtable, node->data.while_data.body);
        break;
    }

    case NODE_FOR_CYCLE: {
        ForData *for_data = &node->data.for_data;
        Type range_type = check_expression(symtable, for_data->range);


        switch (range_type.kind) {
        case TYPE_RANGE:
            for_data->var_type.kind = TYPE_INT;
            break;

        case TYPE_LIST:
            for_data->var_type = *range_type.elem;
            break;

        default:
            error_exit(ERR_SEMANTIC_TYPE, node->line, "expression is not a range");
        }

        scope_add(symtable);
        item_add(symtable, for_data->name, node);

        for (unsigned int i = 0; i < for_data->guards.count; i++) {
            Node *guard = for_data->guards.list[i];
            if (check_expression(symtable, guard).kind != TYPE_BOOL) {
                error_exit(ERR_SEMANTIC_TYPE, guard->line, "guard in for cycle must be boolean");
            }
        }

        check_statement(symtable, for_data->body);
        scope_rm(symtable);
        break;
    }

    case NODE_RETURN: {
        Type expected_type = current_function->data.function_def_data.return_type;
        Node *value = node->data.return_data.value;

        // Return in function with no declared type
        if (expected_type.kind == TYPE_UNKNOWN) {
            error_exit(ERR_SEMANTIC_TYPE, node->line,
                       "return in function %s with no declared return type",
                       current_function->data.function_def_data.name);
        }

        // bare return in non unit function
        if (value == NULL) {
            if (expected_type.kind != TYPE_UNIT) {
                error_exit(ERR_SEMANTIC_TYPE, node->line,
                           "bare return must be in explicitly Unit functions");
            }
            break;
        }

        Type return_type = check_expression(symtable, value);
        if (!type_eq(return_type, expected_type)) {
            error_exit(ERR_SEMANTIC_TYPE, node->line, "type mismatch in return");
        }
        break;
    }

    default:
        check_expression(symtable, node);
        break;
    }
}

void check_function(symtab_t *symtable, Node *function) {
    scope_add(symtable);
    NodeList *parameters = &function->data.function_def_data.params;
    for (unsigned int i = 0; i < parameters->count; i++) {
        Node *param = parameters->list[i];
        if (!item_add(symtable, param->data.param_data.name, param)) {
            error_exit(ERR_SEMANTIC_REDEFINITION, param->line, "redefinition of a parameter %s",
                       param->data.param_data.name);
        }
    }

    current_function = function;
    check_statement(symtable, function->data.function_def_data.body);
    scope_rm(symtable);
}

int semantic_analyze(Node *root) {

    symtab_t *symtable = symtab_init();
    scope_add(symtable);

    // Init heap memory for built-in functions;
    NodeList built_in;
    NodeListInit(&built_in);

    // Add built-in functions
    built_in_function(symtable, &built_in);

    // Collect the functions signatures
    collect_functions(symtable, root);

    // Check existence of only one main function
    check_main(root);

    // Semantic check of all function definitions
    NodeList *function_definitions = &root->data.program_data.definitions;
    for (unsigned int i = 0; i < function_definitions->count; i++) {
        check_function(symtable, function_definitions->list[i]);
    }

    NodeListFree(&built_in);
    symtab_dtor(symtable);
    return 0;
}
