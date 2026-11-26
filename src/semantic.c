/// @file semantic.c
/// @brief Module for semantic analysis
///        Starts from the root and analyzes each function, statement, expression
//
/// @date 12-11-2026
///
/// @author Lukas Sofka (xsofkal00)
/// @author Email: xsofkal00@stud.fit.vutbr.cz
/// Faculty of Information Technology, Brno University of Technology

#include "semantic.h"
#include "ast.h"
#include "error.h"
#include "scanner.h"
#include "symtable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Node *current_function;

typedef struct {
    const char *name;
    Type return_type;
    unsigned int param_count;
    DataType *params;
} BuiltIn;

// clang-format off

/// @brief Declarations of the built-in functions
//  Each function has name, return_type, number of arguments and parameter types
//  Acts as helper structure for the syntactic analyzer
const BuiltIn built_in_functions[] =
    {
        {"print",       {TYPE_UNIT, NULL},   1, (DataType[]){TYPE_ANY}},
        {"readLine",    {TYPE_STRING, NULL}, 0, NULL},
        {"readInt",     {TYPE_INT, NULL},    0, NULL},
        {"readDouble",  {TYPE_DOUBLE, NULL}, 0, NULL},
        {"readBoolean", {TYPE_BOOL, NULL},   0, NULL},
        {"toStr",       {TYPE_STRING, NULL}, 1, (DataType[]){TYPE_ANY}},
        {"toInt",       {TYPE_INT, NULL},    1, (DataType[]){TYPE_DOUBLE}},
        {"strLength",   {TYPE_INT, NULL},    1, (DataType[]){TYPE_STRING}},
        {"subStr",      {TYPE_STRING, NULL}, 3, (DataType[]){TYPE_STRING, TYPE_INT, TYPE_INT}},
        {"ord",         {TYPE_INT, NULL},    2, (DataType[]){TYPE_STRING, TYPE_INT}},
        {"chr",         {TYPE_STRING, NULL}, 1, (DataType[]){TYPE_INT}},
        {"List.range",  {TYPE_LIST, &(Type){TYPE_INT, NULL}}, 3, (DataType[]){TYPE_INT, TYPE_INT, TYPE_INT}},
};

/// @brief Rules for deducing the actual return type
typedef enum {
    RESULT_ELEMENT,  // type of the list's element  (e.g. list.head)
    RESULT_STRING,   // always String               (e.g. list.toString)
    RESULT_LIST_INT, // always List[Int]            (e.g. range.toList)
    RESULT_SAME,     // same type as the left side  (e.g. list.tail, drop, take, slice)
} MemberResultType;


/// @brief Signature of a member used with the dot operator
typedef struct {
    const char *name;            // name of the member
    DataType left;               // type the member belongs to
    int arg_count;               // -1 = written without parentheses, otherwise number of Int arguments
    MemberResultType result_type;   // rule for the result type
} MemberAccessOperators;


/// @brief Member access operators
const MemberAccessOperators member_access_operators[] = {
    {"drop",     TYPE_LIST,    1, RESULT_SAME},
    {"take",     TYPE_LIST,    1, RESULT_SAME},
    {"slice",    TYPE_LIST,    2, RESULT_SAME},
    {"head",     TYPE_LIST,   -1, RESULT_ELEMENT},
    {"tail",     TYPE_LIST,   -1, RESULT_SAME},
    {"toString", TYPE_LIST,   -1, RESULT_STRING},
    {"toList",   TYPE_RANGE,  -1, RESULT_LIST_INT}
};
// clang-format on


/// @brief Returns the type of an identifier from its declaration node
/// @param node
/// @return Type
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


/// @brief Checks whether two types are equal
/// @param a
/// @param b
/// @return true if equal
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


/// @brief Copies a string to newly allocated memory (used in built-in functions)
/// @param str
/// @return Pointer to the copy
char *copy_string(const char *str) {
    char *copy = malloc(strlen(str) + 1);
    if (!copy) {
        error_exit(ERR_INTERNAL, 0, "memory allocation error in copy_string");
    }

    strcpy(copy, str);
    return copy;
}


/// @brief Checks whether the type is Int or Double
/// @note  Helper function
/// @param item
/// @return true if numeric
bool is_numeric(Type item) {
    return (item.kind == TYPE_INT || item.kind == TYPE_DOUBLE);
}


/// @brief Checks whether a type `actual` can be used where type `expected` is required
/// @param expected
/// @param actual
/// @return true if equal, or Int used as Double
bool type_compatible(Type expected, Type actual) {
    return type_eq(expected, actual) || (expected.kind == TYPE_DOUBLE && actual.kind == TYPE_INT);
}


/// @brief Result type of an arithmetic operation on two numeric operands
/// @param a
/// @param b
Type numeric_result(Type a, Type b) {
    Type result = {TYPE_INT, NULL};

    if (a.kind == TYPE_DOUBLE || b.kind == TYPE_DOUBLE) {
        result.kind = TYPE_DOUBLE;
    }
    return result;
}


/// @brief Checks all kinds of statement and returns the type of its value
/// @return Type of the last statement
Type result_type(symtab_t *symtable, Node *node) {
    // Semantic check of the node
    check_statement(symtable, node);

    // If the node is block, then node = last statement
    if (node->kind == NODE_BLOCK) {
        NodeList *statements = &node->data.block_data.statements;

        // Empty block
        if (statements->count == 0) {
            return (Type){TYPE_UNIT, NULL};
        }
        node = statements->list[statements->count - 1];
    }

    // Check whether the last node isn't non returning statement
    switch (node->kind) {
    case NODE_ASSIGN:
    case NODE_VARIABLE_DECLARATION:
    case NODE_WHILE_CYCLE:
    case NODE_FOR_CYCLE:
        // Comes from the specification
        // Blok zakončený definicí proměnné, přiřazením nebo cyklem má výsledek () (typu Unit).
        return (Type){TYPE_UNIT, NULL};

    // Return the type of everything else (if statement, if value, variable, literals, ...)
    default:
        return node->type;
    }
}


/// @brief Creates FUNCTION_DEF nodes for the built-in functions and adds them to the symtable
/// @param built_in_ast
void built_in_function(symtab_t *symtable, NodeList *built_in_ast) {
    // Calculates the count of all built-in functions
    unsigned int count = sizeof(built_in_functions) / sizeof(built_in_functions[0]);

    // Add each to symtable to the scope = 0
    for (unsigned int i = 0; i < count; i++) {
        Node *function = ASTNewNode(NODE_FUNCTION_DEF, 0);
        function->data.function_def_data.return_type = built_in_functions[i].return_type;
        function->data.function_def_data.name = copy_string(built_in_functions[i].name);

        // Copy the parameter nodes
        for (unsigned int j = 0; j < built_in_functions[i].param_count; j++) {
            Node *parameter = ASTNewNode(NODE_PARAMETER, 0);
            parameter->data.param_data.type.kind = built_in_functions[i].params[j];
            NodeListAppend(&function->data.function_def_data.params, parameter);
        }

        item_add(symtable, function->data.function_def_data.name, function);
        NodeListAppend(built_in_ast, function);
    }
}


/// @brief Adds all user functions to the global scope, before any body is checked
/// @note  Also checks the number of main functions
void collect_functions(symtab_t *symtable, Node *root) {
    // Pointer to node function_definition
    Node *main_definition = NULL;
    int main_count = 0;

    // Iterate over all functions
    NodeList *function_definitions = &root->data.program_data.definitions;
    for (unsigned int i = 0; i < function_definitions->count; i++) {
        Node *definition = function_definitions->list[i];
        // If function already exists => error
        if (!item_add(symtable, definition->data.function_def_data.name, definition)) {
            error_exit(ERR_SEMANTIC_REDEFINITION, definition->line, "redefinition of a function %s",
                       definition->data.function_def_data.name);
        }

        // If newly added function is main => update ptr and count
        if (definition->data.function_def_data.is_main) {
            main_definition = function_definitions->list[i];
            main_count++;
        }
    }

    // Checks for the number of main functions
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
}

/// @brief Semantically analyzes given expression and computes its type
/// @note  Checks literals, identifiers, unary or binary operators, list indexing, function call and
///        if expression
/// @param Type of expression
Type check_expression(symtab_t *symtable, Node *node) {
    Type itemType = {TYPE_UNKNOWN, NULL};
    if (!node) {
        return itemType;
    }

    // For literals, return their types
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

            // Checks whether all items are of the same type
            // If element_type is still TYPE_UNKNOWN => set the first type
            if (element_type.kind == TYPE_UNKNOWN) {
                element_type = item_type;
            } else if (is_numeric(element_type) && is_numeric(item_type)) {
                element_type = numeric_result(element_type, item_type);
            }
            // Checks whether the declared type is equal to the given one
            else if (item_type.kind != TYPE_UNKNOWN && !type_eq(element_type, item_type)) {
                error_exit(ERR_SEMANTIC_TYPE, items->list[i]->line,
                           "declaration of list with mismatching types, expected %s, got %s",
                           DataTypeToString(element_type.kind), DataTypeToString(item_type.kind));
            }
        }

        // Expression's type is List[element_type]
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

        // Check whether a function was used as a variable
        if (item.decl->kind == NODE_FUNCTION_DEF) {
            error_exit(ERR_SEMANTIC_TYPE, node->line, "function %s used as variable",
                       node->data.variable_name);
        }

        itemType = declaration_to_type(item.decl);
        break;
    }

    case NODE_BINARY_OPERATOR: {
        BinaryOpData *bin_data = &node->data.bin_op_data;

        // Special case for Member Access
        if (bin_data->op == TOKEN_DOT) {
            // Left side of the member access needs to be parsed beforehand
            // e.g. List.slice(...).head => (List.slice).head
            Type left = check_expression(symtable, bin_data->left);
            Node *right = bin_data->right;

            const char *name = NULL;
            NodeList *arguments = NULL;

            // Member access is either identifier or function call
            if (right->kind == NODE_ID) {
                name = right->data.variable_name;
            } else if (right->kind == NODE_FUNCTION_CALL) {
                name = right->data.call_data.name;
                arguments = &right->data.call_data.arguments;
            } else {
                error_exit(ERR_SEMANTIC_TYPE, node->line, "invalid member access");
            }

            // Find the correct access member
            const MemberAccessOperators *result = NULL;
            unsigned int dot_op_count =
                sizeof(member_access_operators) / sizeof(member_access_operators[0]);

            for (unsigned int i = 0; i < dot_op_count; i++) {

                // Check left type and right name against the defined dot operators
                if (member_access_operators[i].left == left.kind &&
                    strcmp(member_access_operators[i].name, name) == 0) {
                    result = &member_access_operators[i];
                    break;
                }
            }

            // If no member access found => error
            if (!result) {
                error_exit(ERR_SEMANTIC_UNDEFINED, node->line,
                           "undefined dot operator %s used on %s", name,
                           DataTypeToString(left.kind));
            }

            // Check whether arguments match
            if (result->arg_count == -1) {
                // If no args are expected and some are given => error
                if (arguments) {
                    error_exit(
                        ERR_SEMANTIC_TYPE, node->line,
                        "dot operator `%s` takes no arguments (write it without parentheses)",
                        name);
                }
            } else {
                // If some args are expected and none is given => error
                if (!arguments) {
                    error_exit(ERR_SEMANTIC_TYPE, node->line,
                               "dot operator `%s` expects %i arguments, got none", name,
                               result->arg_count);
                    // expected count doesn't match given count
                } else if ((int)arguments->count != result->arg_count) {
                    error_exit(ERR_SEMANTIC_ARGUMENTS, node->line,
                               "dot operator `%s` expects %i arguments, got %i", name,
                               result->arg_count, arguments->count);
                }

                for (unsigned int i = 0; i < arguments->count; i++) {
                    if (check_expression(symtable, arguments->list[i]).kind != TYPE_INT) {
                        error_exit(ERR_SEMANTIC_ARGUMENTS, node->line, "invalid argument type");
                    }
                }
            }

            // Based on the member access structure find the return itemType
            switch (result->result_type) {
            case RESULT_ELEMENT:
                itemType = *left.elem;
                break;

            case RESULT_SAME:
                itemType = left;
                break;

            case RESULT_STRING:
                itemType.kind = TYPE_STRING;
                break;

            case RESULT_LIST_INT:
                itemType.kind = TYPE_LIST;
                itemType.elem = TypeInit(node->line);
                *itemType.elem = (Type){TYPE_INT, NULL};
                break;

            default:
                error_exit(ERR_SEMANTIC_TYPE, node->line, "undefined return argument in %s", name);
            }

            break;
        }

        // Rest of the binary operators go here
        Type left = check_expression(symtable, bin_data->left);
        Type right = check_expression(symtable, bin_data->right);
        Type num_result = numeric_result(left, right);

        // Switch between all kinds of binary operators
        switch (bin_data->op) {

        // numeric + numeric
        // String + String
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

        // numeric (-, *, /) numeric
        case TOKEN_MINUS:
        case TOKEN_MUL:
        case TOKEN_DIV:
            if (!is_numeric(left) || !is_numeric(right)) {
                error_exit(ERR_SEMANTIC_TYPE, node->line, "operands must be numeric");
            }

            itemType = num_result;
            break;

        // numeric (<, <=, >, >=) numeric
        // String  (<, <=, >, >=) String
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

        // numeric (==, !=) numeric
        // T (==, !=) T (T = any types)
        case TOKEN_EQ:
        case TOKEN_NEQ:
            if ((is_numeric(left) && is_numeric(right)) || type_eq(left, right)) {
                itemType.kind = TYPE_BOOL;
            } else {
                error_exit(ERR_SEMANTIC_TYPE, node->line,
                           "operands must be numeric or of the same type");
            }
            break;

        // Boolean (&&, ||) Boolean
        case TOKEN_AND:
        case TOKEN_OR:
            if (left.kind == TYPE_BOOL && right.kind == TYPE_BOOL) {
                itemType.kind = TYPE_BOOL;
            } else {
                error_exit(ERR_SEMANTIC_TYPE, node->line, "operands must be boolean");
            }
            break;

        // List[T] ++ List[T]
        // String  ++ String
        case TOKEN_CONCATENATION:
            // If doesn't match or left is neither TYPE_LIST nor TYPE_STRING => error
            if (!type_eq(left, right) || (left.kind != TYPE_LIST && left.kind != TYPE_STRING)) {
                error_exit(
                    ERR_SEMANTIC_TYPE, node->line,
                    "both operands of concatenation must be lists of the same type or strings");
            }

            itemType = left;
            break;

        // Range by Int
        case TOKEN_BY:
            if (left.kind != TYPE_RANGE || right.kind != TYPE_INT) {
                error_exit(ERR_SEMANTIC_TYPE, node->line,
                           "operands for operation `by` must be range and integer");
            } else if (bin_data->right->kind == NODE_INT && bin_data->right->data.int_value == 0) {
                error_exit(ERR_SEMANTIC_OTHER, node->line,
                           "zero step given in for cycle is not permitted");
            }

            itemType.kind = TYPE_RANGE;
            break;

        // Int (until, to) Int
        case TOKEN_UNTIL:
        case TOKEN_TO:
            if (left.kind != TYPE_INT || right.kind != TYPE_INT) {
                error_exit(ERR_SEMANTIC_TYPE, node->line,
                           "operands for operation `range` must be integers");
            }

            itemType.kind = TYPE_RANGE;
            break;

        default:
            break;
        }

        break;
    }

    // Case for all 3 unary operators (+,-,!)
    case NODE_UNARY_OPERATOR: {
        Type type = check_expression(symtable, node->data.un_op_data.operand);
        unsigned int op = node->data.un_op_data.op;

        // Switch between the 3 possibilities
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

    // Checks the function_calls and list indexing
    // Check type match between all parameters and arguments
    // Argument has to be compatible with the parameter
    case NODE_FUNCTION_CALL: {
        item_t item = get_item(symtable, node->data.call_data.name);

        // Check whether the called function exists
        if (item.scope == -1) {
            error_exit(ERR_SEMANTIC_UNDEFINED, node->line, "undefined function `%s`",
                       node->data.call_data.name);
        }

        // Check function call
        if (item.decl->kind == NODE_FUNCTION_DEF) {

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

                if (!type_compatible(param_type, arg_type) && param_type.kind != TYPE_ANY) {
                    error_exit(ERR_SEMANTIC_ARGUMENTS, args->list[i]->line,
                               "argument %u of %s has a wrong type", i + 1,
                               node->data.call_data.name);
                }
            }

            // Function On-demand Inference
            // Definition of a function called from main is not processed yet -> check_function call
            FunctionDefData *function = &item.decl->data.function_def_data;
            if (function->return_type.kind == TYPE_UNDEF) {
                if (function->function_check == FunctionBeingChecked) {
                    error_exit(ERR_SEMANTIC_TYPE, node->line,
                               "cannot infer the return type in recursive function %s",
                               node->data.call_data.name);
                } else {
                    check_function(symtable, item.decl);
                }
            }

            // Hard coded test for zero step for function List.range
            if (strcmp(node->data.call_data.name, "List.range") == 0) {
                Node *step = node->data.call_data.arguments.list[2];
                if (step->kind == NODE_INT && step->data.int_value == 0) {
                    error_exit(ERR_SEMANTIC_OTHER, node->line, "zero step in List.range");
                }
            }

            itemType = item.decl->data.function_def_data.return_type;

        } else if (declaration_to_type(item.decl).kind == TYPE_LIST) {
            // List indexing
            // Finds the defined type for the list
            // And gets all arguments, expects only 1
            Type defined_type = declaration_to_type(item.decl);
            NodeList *arguments = &node->data.call_data.arguments;

            // If indexing with multiple indices => error
            if (arguments->count != 1) {
                error_exit(ERR_SEMANTIC_ARGUMENTS, node->line, "list `%s` expects 1 index, got %i",
                           node->data.call_data.name, arguments->count);
            }

            // Index must be of a type Integer
            Type index = check_expression(symtable, arguments->list[0]);
            if (index.kind != TYPE_INT) {
                error_exit(ERR_SEMANTIC_TYPE, node->line,
                           "index of a list `%s` must be of type Int", node->data.call_data.name);
            }

            itemType = *defined_type.elem;

        } else {
            error_exit(ERR_SEMANTIC_TYPE, node->line, "TOOD");
        }

        break;
    }

    // Both branches in if_value have to have the same type
    case NODE_IF_VALUE: {
        Type condinition = check_expression(symtable, node->data.if_data.condition);
        if (condinition.kind != TYPE_BOOL) {
            error_exit(ERR_SEMANTIC_TYPE, node->line, "condition is not of type boolean");
        }

        Type then_type = result_type(symtable, node->data.if_data.then_part);
        Type else_type = result_type(symtable, node->data.if_data.else_part);

        // If both branches are numerical => implicit conversion
        if (type_eq(then_type, else_type)) {
            itemType = then_type;
        } else if (is_numeric(then_type) && is_numeric(else_type)) {
            itemType = numeric_result(then_type, else_type);
        } else {
            error_exit(ERR_SEMANTIC_TYPE, node->line,
                       "both branches must be of the same type or numeric");
        }
        break;
    }

    default:
        break;
    }

    node->type = itemType;
    return itemType;
}


/// @brief Semantically analyzes given statement
///        Checks all blocks, variable declarations, assignments, if statements, for loops, while
///        loops, returns
void check_statement(symtab_t *symtable, Node *node) {
    if (!node) {
        return;
    }

    // Switch between all possible node kinds of type statement
    switch (node->kind) {
    case NODE_BLOCK:
        // Block statements need additional scope
        scope_add(symtable);

        // For each statement in block call check_statement
        NodeList *statemenst = &node->data.block_data.statements;
        for (unsigned int i = 0; i < statemenst->count; i++) {
            check_statement(symtable, statemenst->list[i]);
        }
        scope_rm(symtable);
        break;

    // e.g. val i: Int
    case NODE_VARIABLE_DECLARATION: {
        // Find the types
        Type *declared_type = &node->data.var_decl_data.type;
        Type value_type = check_expression(symtable, node->data.var_decl_data.value);

        // Special case for List[Double] = List(1,2)
        Node *value = node->data.var_decl_data.value;
        if (value->kind == NODE_LIST_LITERAL) {
            if (declared_type->kind == TYPE_LIST && declared_type->elem->kind == TYPE_DOUBLE &&
                value->type.kind == TYPE_LIST && value->type.elem->kind == TYPE_INT) {
                value->type.elem->kind = TYPE_DOUBLE;
            }
        }

        // Empty list expects given type (page 15)
        if (declared_type->kind == TYPE_UNDEF) {
            if (value_type.kind == TYPE_LIST && value_type.elem->kind == TYPE_UNKNOWN) {
                error_exit(ERR_SEMANTIC_TYPE, node->line,
                           "cannot infer type of %s, declare it (e.g. val x: List[Int] = List ())",
                           node->data.var_decl_data.name);
            }

            *declared_type = value_type;

            // If declared type doesn't match the value type
        } else if (!type_compatible(*declared_type, value_type)) {
            error_exit(ERR_SEMANTIC_TYPE, node->line, "type mismatch in declaration of %s",
                       node->data.var_decl_data.name);
        }

        // If symbol already exists => error
        if (!item_add(symtable, node->data.var_decl_data.name, node)) {
            error_exit(ERR_SEMANTIC_REDEFINITION, node->line, "redefinition of a variable %s",
                       node->data.var_decl_data.name);
        }
        break;
    }

    // e.g. id = expression;
    case NODE_ASSIGN: {
        // Get the identifier
        item_t item = get_item(symtable, node->data.assign_data.name);

        // If not found => error
        if (item.scope == -1) {
            error_exit(ERR_SEMANTIC_UNDEFINED, node->line, "undefined variable %s",
                       node->data.assign_data.name);
        }

        // Identifiers may come from different nodes (variable_declaration, for cycle, function
        // def, parameter)
        switch (item.decl->kind) {
        case NODE_VARIABLE_DECLARATION:
            // If identifier is value => error
            if (item.decl->data.var_decl_data.is_val) {
                error_exit(ERR_SEMANTIC_IMMUTABLE, node->line, "assignment to value %s",
                           node->data.assign_data.name);
            }
            break;

        // Cannot write to param
        case NODE_PARAMETER:
            error_exit(ERR_SEMANTIC_IMMUTABLE, node->line, "parameter %s is immutable",
                       item.decl->data.param_data.name);
            break;

        // Cannot write to for cycle id
        case NODE_FOR_CYCLE:
            error_exit(ERR_SEMANTIC_IMMUTABLE, node->line, "variable %s in for cycle is immutable",
                       item.decl->data.for_data.name);
            break;

        // Cannot write to function id
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
        if (!type_compatible(var_type, value_type)) {
            error_exit(ERR_SEMANTIC_TYPE, node->line, "type mismatch in assignment to %s",
                       node->data.assign_data.name);
        }
        break;
    }

    // e.g. if expr1 then expr2 else if expr3 else expr4
    case NODE_IF_STATEMENT: {
        // Check the condition, if not bool => error
        Type condition = check_expression(symtable, node->data.if_data.condition);
        if (condition.kind != TYPE_BOOL) {
            error_exit(ERR_SEMANTIC_TYPE, node->line, "condition is not of type boolean");
        }

        // Then part is mandatory, else is not
        Type then_type = result_type(symtable, node->data.if_data.then_part);
        Type else_type = {TYPE_UNIT, NULL};

        // If else node exists =>
        if (node->data.if_data.else_part != NULL) {
            else_type = result_type(symtable, node->data.if_data.else_part);
        }

        if (type_eq(then_type, else_type)) {
            node->type = then_type;
        } else if (is_numeric(then_type) && is_numeric(else_type)) {
            node->type = numeric_result(then_type, else_type);
        } else {
            node->type = (Type){TYPE_UNIT, NULL};
        }

        break;
    }

    // e.g. while i > 10 do body
    case NODE_WHILE_CYCLE: {
        // Check the condition, if not bool => error
        Type condition = check_expression(symtable, node->data.while_data.condition);
        if (condition.kind != TYPE_BOOL) {
            error_exit(ERR_SEMANTIC_TYPE, node->line, "condition is not of type boolean");
        }

        // Just check the body correctness
        check_statement(symtable, node->data.while_data.body);
        break;
    }

    // e.g. for i <- range
    //          guard1 do
    //          body
    case NODE_FOR_CYCLE: {
        // Check the range expression
        ForData *for_data = &node->data.for_data;
        Type range_type = check_expression(symtable, for_data->range);


        // Type of variable depends on the iterated object
        // range => int, List[T] => T
        switch (range_type.kind) {
        case TYPE_RANGE:
            for_data->var_type.kind = TYPE_INT;
            break;

        case TYPE_LIST:
            if (range_type.elem->kind == TYPE_UNKNOWN) {
                error_exit(ERR_SEMANTIC_TYPE, node->line, "list has no element type");
            }

            for_data->var_type = *range_type.elem;
            break;

        default:
            error_exit(ERR_SEMANTIC_TYPE, node->line, "expression is not a range");
        }

        // Add the variable name
        scope_add(symtable);
        item_add(symtable, for_data->name, node);

        // Every guard must be boolean => check_expression
        for (unsigned int i = 0; i < for_data->guards.count; i++) {
            Node *guard = for_data->guards.list[i];
            if (check_expression(symtable, guard).kind != TYPE_BOOL) {
                error_exit(ERR_SEMANTIC_TYPE, guard->line, "guard in for cycle must be boolean");
            }
        }

        // Check the body
        check_statement(symtable, for_data->body);
        scope_rm(symtable);
        break;
    }

    // e.g. return expression
    case NODE_RETURN: {
        Type expected_type = current_function->data.function_def_data.return_type;
        Node *value_ptr = node->data.return_data.value;

        // Return in function with no declared type
        if (expected_type.kind == TYPE_UNDEF) {
            error_exit(ERR_SEMANTIC_TYPE, node->line,
                       "return in function %s with no declared return type",
                       current_function->data.function_def_data.name);
        }

        // Bare return in non unit function
        if (value_ptr == NULL) {
            if (expected_type.kind != TYPE_UNIT) {
                error_exit(ERR_SEMANTIC_TYPE, node->line,
                           "bare return must be in explicitly Unit functions");
            }
            break;
        }

        // Check the return type of the value in NODE_RETURN
        Type return_type = check_expression(symtable, value_ptr);

        // Functions with Unit return type evaluate the value but discards it
        if (expected_type.kind != TYPE_UNIT && !type_compatible(expected_type, return_type)) {
            error_exit(ERR_SEMANTIC_TYPE, node->line, "type mismatch in return");
        }

        node->type = expected_type;
        break;
    }

    default:
        check_expression(symtable, node);
        break;
    }
}

/// @brief Semantically analyzes given function
void check_function(symtab_t *symtable, Node *function) {
    // If we already analyzed the function => skip
    if (function->data.function_def_data.function_check == FunctionChecked) {
        return;
    }
    function->data.function_def_data.function_check = FunctionBeingChecked;

    scope_add(symtable);
    // Check every parameter
    NodeList *parameters = &function->data.function_def_data.params;
    for (unsigned int i = 0; i < parameters->count; i++) {
        Node *param = parameters->list[i];

        // For each function's parameter check whether the name repeats -> redefinition
        if (!item_add(symtable, param->data.param_data.name, param)) {
            error_exit(ERR_SEMANTIC_REDEFINITION, param->line, "redefinition of a parameter %s",
                       param->data.param_data.name);
        }
    }

    // Change current_function to the currently parsed function
    current_function = function;

    // Function return type checking
    Type body_type = result_type(symtable, function->data.function_def_data.body);
    Type *declared = &function->data.function_def_data.return_type;

    // Function with undefined return type infers the type from return
    // If the type from return is undefined list => error
    if (declared->kind == TYPE_UNDEF) {
        if (body_type.kind == TYPE_LIST && body_type.elem->kind == TYPE_UNKNOWN) {
            error_exit(ERR_SEMANTIC_TYPE, function->line, "cannot infer type of %s[%s], declare it",
                       DataTypeToString(body_type.kind), DataTypeToString(body_type.elem->kind));
        }
        *declared = body_type;
    }
    // If the function does return the value and they are not compatible -> error
    else if (declared->kind != TYPE_UNIT && !type_compatible(*declared, body_type)) {
        error_exit(ERR_SEMANTIC_TYPE, function->line, "function `%s` returns wrong type",
                   function->data.function_def_data.name);
    }

    // @main function has to return Unit
    if (function->data.function_def_data.is_main && declared->kind != TYPE_UNIT) {
        error_exit(ERR_SEMANTIC_TYPE, function->line, "main function must return Unit, not %s",
                   DataTypeToString(declared->kind));
    }

    function->data.function_def_data.function_check = FunctionChecked;
    scope_rm(symtable);
}

/// @brief Starting point of the semantic analysis
int semantic_analyze(Node *root) {
    // Init symtable
    symtab_t *symtable = symtab_init();
    scope_add(symtable);

    // Init heap memory for built-in functions;
    NodeList built_in;
    NodeListInit(&built_in);

    // Add built-in functions
    built_in_function(symtable, &built_in);

    // Collect the function signatures
    collect_functions(symtable, root);

    // Semantic check of all function definitions
    NodeList *function_definitions = &root->data.program_data.definitions;
    for (unsigned int i = 0; i < function_definitions->count; i++) {
        check_function(symtable, function_definitions->list[i]);
    }

    // Free built-in functions and symtable
    NodeListFree(&built_in);
    symtab_dtor(symtable);
    return 0;
}
