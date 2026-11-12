// @file ast.h
// @brief Header file of Abstract Syntax Tree with all nodes types and functions declarations
// @date 12-11-2026
//
// @author Lukas Sofka (xsofkal00)
// @author Email: xsofkal00@stud.fit.vutbr.cz
// Faculty of Information Technology, Brno University of Technology
// clang-format off

#ifndef AST_H
#define AST_H

#include "scanner.h"
#include <stdbool.h>

#define INITIAL_LIST_CAPACITY 4

typedef struct Node Node;

// @brief Represents the Nodes purpose
typedef enum {
    NODE_PROGRAM,
    NODE_FUNCTION_DEF,
    NODE_PARAMETER,
    NODE_BLOCK,
    NODE_VARIABLE_DECLARATION,
    NODE_ASSIGN,
    NODE_IF_STATEMENT,
    NODE_FOR_CYCLE,
    NODE_WHILE_CYCLE,
    NODE_RETURN,
    NODE_BINARY_OPERATOR,
    NODE_UNARY_OPERATOR,
    NODE_IF_VALUE,
    NODE_LIST_LITERAL,
    NODE_EXPRESSION,
    NODE_FUNCTION_CALL,
    NODE_ID,
    NODE_INT,
    NODE_DOUBLE,
    NODE_BOOLEAN,
    NODE_STRING,
    NODE_UNIT,
} NodeKind;

// @brief Represents all possible datatypes for our language
typedef enum {
    TYPE_UNKNOWN,
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_UNIT,
    TYPE_LIST,
} DataType;


// Core blocks

// @brief Reprents the data type of an statement, variable, expression, ...
// Is implemented with additional elem pointer for the item type of List[Int]
//
// Note: elem is used only for TYPE_LIST, otherwise NULL
//
// Example:
// List(2)
// .kind = NODE_LIST_LITERAL
// .type = List[Int] (after semantic analysis)
// .data.list.items[0] -> .kind = NODE_INT, .data.int_val = 2

typedef struct Type {
    DataType kind;
    struct Type *elem;
} Type;

typedef struct NodeList {
    Node **list;
    unsigned int capacity;
    unsigned int count;
} NodeList;


// Top level
// Represents the highest nodes - BlockData, Program, Function Definitions Data and Parameters Data
typedef struct {
    char *name;
    Type type;
} ParamData;

typedef struct {
    char *name;
    bool is_main;
    NodeList params;
    Node *body;
    Type return_type;
} FunctionDefData;

typedef struct {
    NodeList definitions;
} ProgramData;

// Statements
typedef struct {
    NodeList statements;
} BlockData;

typedef struct {
    char *name;
    Type type;
    bool is_val;
    Node *value;
} VarDeclarationData;

typedef struct {
    char *name;
    Node *value;
} AssignData;

typedef struct {
    Node *condition;
    Node *then_part;
    Node *else_part;
} IfData;

typedef struct {
    Node *condition;
    Node *body;
} WhileData;

typedef struct {
    char *name;
    Node *range;
    Node *body;
    NodeList guards;
} ForData;

typedef struct {
    Node *value;
} ReturnData;

// Expressions
typedef struct {
    TokenType op;
    Node *left;
    Node *right;
} BinaryOpData;

typedef struct {
    TokenType op;
    Node *operand;
} UnaryOpData;

typedef struct {
    char *name;
    NodeList arguments;
} CallData;

typedef struct {
    NodeList items;
} ListData;


// @brief  Universal Node
// kind     = what kind of purpose does the node fulfill (see NodeKind enum)
// line     = line in the source where the construct starts
// type     = type of the value the expression produces (e.g. 1 + a is Int or Double, depending on a), filled later by semantic analysis, until then we assign TYPE_UNKNOWN
// data     = fields specific to the kind
struct Node {
    NodeKind kind;
    unsigned line;
    Type type;

    union {
        ProgramData program_data;
        FunctionDefData function_def_data;
        ParamData param_data;
        BlockData block_data;
        VarDeclarationData var_decl_data;
        AssignData assign_data;
        IfData if_data;
        WhileData while_data;
        ForData for_data;
        ReturnData return_data;
        BinaryOpData bin_op_data;
        UnaryOpData un_op_data;
        CallData call_data;
        ListData list_data;
        Node *expression_data;

        char *variable_name;
        int int_value;
        double double_value;
        char *string_val;
        bool bool_value;
    } data;
};

Node *ASTNewNode(NodeKind kind, unsigned int line);
void ASTFree(Node *ptr);
void NodeListInit(NodeList *ptr);
void NodeListFree(NodeList *ptr);
void NodeListAppend(NodeList *ptr, Node *node);

#endif
