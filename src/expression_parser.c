/// @file expression_parser.c
/// @brief Precedence parser for expressions
/// @date 07-11-2026
///
/// @author Lukas Sofka (xsofkal00)
/// @author Email: xsofkal00@stud.fit.vutbr.cz
/// Faculty of Information Technology, Brno University of Technology

#include "expression_parser.h"
#include "ast.h"
#include "error.h"
#include "parser.h"
#include "scanner.h"

#include <stdbool.h>
#include <stdlib.h>


// clang-format off
typedef enum {
    S_OPERAND, // variable, double, int, string, ...
    S_LPAREN,  // (
    S_RPAREN,  // )
    S_DOT,     // .
    S_UNARY,   // -, !, +
    S_MUL,     // *, /
    S_ADD,     // +, -, ++
    S_REL,     // <=, >=, >, <
    S_EQ,      // ==, !=
    S_AND,     // &&
    S_OR,      // ||
    S_RANGE,   // to, until, by
    S_END,     // $
    S_COUNT,   // Count of terminals
    S_EXPR,    // E
    S_MARK     // <
} Symbol;

typedef struct {
    Symbol symbol; // Holds the symbol (as previously)
    TokenType op;  // Holds the token type (since symbol doesn't carry the same information as TokenType)
    Node *node;    // Pointer to subtress (e.g. UNARY_OP ->  OPERAND)
} StackItem;

typedef struct AStack {
    StackItem *items;
    int count;
    int capacity;
} Stack;


/// @brief  Precedence Table
/// row     - topmost terminal on the stack
/// columnt - current input symbol
/// <       - shift with mark
/// =       - shift without mark
/// >       - reduce
/// ' '     - syntax error
const char precedence_table[S_COUNT][S_COUNT] = {
    //                 i    (    )    .    !    *    +    >   ==   &&   ||   to    $
    [S_OPERAND]  =  { ' ', ' ', '>', '>', ' ', '>', '>', '>', '>', '>', '>', '>', '>'},
    [S_LPAREN]   =  { '<', '<', '=', '<', '<', '<', '<', '<', '<', '<', '<', '<', ' '},
    [S_RPAREN]   =  { ' ', ' ', '>', '>', ' ', '>', '>', '>', '>', '>', '>', '>', '>'},
    [S_DOT]      =  { '<', ' ', '>', '>', ' ', '>', '>', '>', '>', '>', '>', '>', '>'},
    [S_UNARY]    =  { '<', '<', '>', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>'},
    [S_MUL]      =  { '<', '<', '>', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>'},
    [S_ADD]      =  { '<', '<', '>', '<', '<', '<', '>', '>', '>', '>', '>', '>', '>'},
    [S_REL]      =  { '<', '<', '>', '<', '<', '<', '<', '>', '>', '>', '>', '>', '>'},
    [S_EQ]       =  { '<', '<', '>', '<', '<', '<', '<', '<', '>', '>', '>', '>', '>'},
    [S_AND]      =  { '<', '<', '>', '<', '<', '<', '<', '<', '<', '>', '>', '>', '>'},
    [S_OR]       =  { '<', '<', '>', '<', '<', '<', '<', '<', '<', '<', '>', '>', '>'},
    [S_RANGE]    =  { '<', '<', '>', '<', '<', '<', '<', '<', '<', '<', '<', '>', '>'},
    [S_END]      =  { '<', '<', ' ', '<', '<', '<', '<', '<', '<', '<', '<', '<', ' '}
};
// clang-format on


/// @brief Initializes the stack with capacity set with STACK_INITIAL_CAPACITY
void initStack(Stack *stack) {
    stack->capacity = STACK_INITIAL_CAPACITY;
    stack->count = 0;
    stack->items = malloc(sizeof(StackItem) * stack->capacity);
    if (!stack->items) {
        error_exit(ERR_INTERNAL, 0, "Memory allocation fail: initStack");
    }
}

/// @brief Free stack from memory
void freeStack(Stack *stack) {
    free(stack->items);
}

/// @brief Push an element to stack
void pushStack(Stack *stack, StackItem symbol) {
    if (stack->count == stack->capacity) {
        stack->capacity *= 2;
        StackItem *realloc_array = realloc(stack->items, stack->capacity * sizeof(StackItem));
        if (!realloc_array) {
            error_exit(ERR_INTERNAL, 0, "Memory allocation fail: pushStack");
        }
        stack->items = realloc_array;
    }

    stack->items[stack->count] = symbol;
    stack->count++;
}

/// @brief Pop topmost element and return it
StackItem popStack(Stack *stack) {
    stack->count--;
    return stack->items[stack->count];
}

/// @brief Predicate whether stack is empty
bool isEmptyStack(Stack *stack) {
    return stack->count == 0;
}

/// @brief Returns the first terminal on the stack
/// Skips S_MARK and S_EXPR
StackItem topTerminal(Stack *stack) {
    int i = stack->count - 1;
    while (stack->items[i].symbol >= S_COUNT) {
        i--;
    }

    return stack->items[i];
}

/// @brief Inserts S_MARK right after the given terminal
void insertMark(Stack *stack, StackItem terminal) {
    Stack temp;
    initStack(&temp);

    while (!isEmptyStack(stack) && stack->items[stack->count - 1].symbol != terminal.symbol) {
        pushStack(&temp, popStack(stack));
    }

    StackItem mark = {S_MARK, 0, NULL};
    pushStack(stack, mark);

    while (!isEmptyStack(&temp)) {
        pushStack(stack, popStack(&temp));
    }

    freeStack(&temp);
}

/// @brief Predicate whether the end condition is met
/// End condition: $E
bool isExpressionFinished(Stack *stack) {
    return stack->count == 2 && stack->items[0].symbol == S_END && stack->items[1].symbol == S_EXPR;
}

/// @brief Converts the current token to a symbol
Symbol tokenToSymbol(void) {
    switch (current.type) {
    case TOKEN_ID:
    case TOKEN_INT:
    case TOKEN_DOUBLE:
    case TOKEN_STRING:
    case TOKEN_TRUE:
    case TOKEN_FALSE:
    case TOKEN_LIST:
        return S_OPERAND;

    case TOKEN_LPAREN:
        return S_LPAREN;

    case TOKEN_RPAREN:
        return S_RPAREN;

    case TOKEN_DOT:
        return S_DOT;

    case TOKEN_NOT:
        return S_UNARY;

    case TOKEN_PLUS:
    case TOKEN_MINUS:
        switch (previous.type) {
        case TOKEN_ID:
        case TOKEN_INT:
        case TOKEN_DOUBLE:
        case TOKEN_STRING:
        case TOKEN_TRUE:
        case TOKEN_FALSE:
        case TOKEN_RPAREN:
            return S_ADD;
        default:
            return S_UNARY;
        }

    case TOKEN_MUL:
    case TOKEN_DIV:
        return S_MUL;

    case TOKEN_CONCATENATION:
        return S_ADD;

    case TOKEN_LS:
    case TOKEN_GR:
    case TOKEN_LS_EQ:
    case TOKEN_GR_EQ:
        return S_REL;

    case TOKEN_EQ:
    case TOKEN_NEQ:
        return S_EQ;

    case TOKEN_AND:
        return S_AND;
    case TOKEN_OR:
        return S_OR;

    case TOKEN_TO:
    case TOKEN_UNTIL:
    case TOKEN_BY:
        return S_RANGE;

    case TOKEN_IF:
        switch (previous.type) {
        case TOKEN_ID:
        case TOKEN_INT:
        case TOKEN_DOUBLE:
        case TOKEN_STRING:
        case TOKEN_TRUE:
        case TOKEN_FALSE:
        case TOKEN_RPAREN:
            return S_END;
        default:
            return S_OPERAND;
        }

    default:
        return S_END;
    }
}

/// @brief Reduces the handle to S_EXPR
/// Rule
/// E -> E op E    New Binary Operator
/// E -> unary E   New Unary Operator
/// E -> i         New Leaf
/// E -> ( E )     Parentheses are skipped in creation of AST
void reduce(Stack *stack) {
    // Find the topmost mark, the handle starts right after it
    int mark_idx = stack->count - 1;
    while (stack->items[mark_idx].symbol != S_MARK) {
        mark_idx--;
    }

    // The handle points directly into the stack, its size is 1, 2 or 3 items
    StackItem *handle = &stack->items[mark_idx + 1];
    int size = stack->count - mark_idx - 1;

    Node *ptr = NULL;
    bool valid = false;
    switch (size) {
    case 1:
        if (handle[0].symbol == S_OPERAND) {
            valid = true;
            ptr = handle[0].node;
        }
        break;

    case 2:
        if (handle[0].symbol == S_UNARY && handle[1].symbol == S_EXPR) {
            valid = true;

            // Create a UNARY_OPERATOR node with the operator (!, - or +) and its operand
            ptr = ASTNewNode(NODE_UNARY_OPERATOR, handle[1].node->line);
            ptr->data.un_op_data.operand = handle[1].node;
            ptr->data.un_op_data.op = handle[0].op;
        }
        break;

    case 3:
        // clang-format off
        if (handle[0].symbol == S_LPAREN && handle[1].symbol == S_EXPR && handle[2].symbol == S_RPAREN) {
            ptr = handle[1].node;
            valid = true;

        } else if (handle[0].symbol == S_EXPR &&
                   (handle[1].symbol == S_DOT || handle[1].symbol == S_ADD || handle[1].symbol == S_MUL ||
                    handle[1].symbol == S_EQ  || handle[1].symbol == S_REL || handle[1].symbol == S_AND ||
                    handle[1].symbol == S_OR  || handle[1].symbol == S_RANGE) &&
                   handle[2].symbol == S_EXPR) {
            valid = true;

            // Create a BINARY_OPERATOR node with the operator and left and right subtrees
            ptr = ASTNewNode(NODE_BINARY_OPERATOR, handle[0].node->line);
            ptr->data.bin_op_data.left = handle[0].node;
            ptr->data.bin_op_data.op = handle[1].op;
            ptr->data.bin_op_data.right = handle[2].node;

            // clang-format on
        }
        break;

    default:
        syntax_error("expression");
    }

    if (!valid) {
        syntax_error("expression");
    }

    stack->count = mark_idx;

    StackItem expr = {S_EXPR, 0, ptr};
    pushStack(stack, expr);
}

/// @brief Predicate whether there is S_LPAREN on the stack
bool hasOpenParen(Stack *stack) {
    for (int i = 0; i < stack->count; i++) {
        if (stack->items[i].symbol == S_LPAREN) {
            return true;
        }
    }
    return false;
}

/// @brief
Node *LeafNode(void) {
    Node *ptr = NULL;

    switch (current.type) {
    case TOKEN_ID:
        ptr = ASTNewNode(NODE_ID, current.line);
        ptr->data.variable_name = current.value.str_val;
        current.value.str_val = NULL;
        break;

    case TOKEN_INT:
        ptr = ASTNewNode(NODE_INT, current.line);
        ptr->data.int_value = current.value.int_val;
        break;

    case TOKEN_DOUBLE:
        ptr = ASTNewNode(NODE_DOUBLE, current.line);
        ptr->data.double_value = current.value.double_val;
        break;

    case TOKEN_STRING:
        ptr = ASTNewNode(NODE_STRING, current.line);
        ptr->data.string_val = current.value.str_val;
        current.value.str_val = NULL;
        break;

    case TOKEN_TRUE:
        ptr = ASTNewNode(NODE_BOOLEAN, current.line);
        ptr->data.bool_value = true;
        break;

    case TOKEN_FALSE:
        ptr = ASTNewNode(NODE_BOOLEAN, current.line);
        ptr->data.bool_value = false;
        break;

    default:
        syntax_error("operand");
    }
    return ptr;
}

/// @brief Parses one expression using the precedence table
/// Stops at the first token that is not part of an expression
Node *parse_expression(void) {
    // Init stack
    Stack stack;
    initStack(&stack);

    // Push $ to the stack
    StackItem end = {S_END, 0, NULL};
    pushStack(&stack, end);

    StackItem top_terminal;
    Symbol input;

    while (true) {
        // Get the topmost terminal and current symbol (in presentations a,b)
        top_terminal = topTerminal(&stack);
        input = tokenToSymbol();


        // The S_LPAREN of a function call is not pushed, so rparen with no lparen ends expression
        if (input == S_RPAREN && !hasOpenParen(&stack)) {
            input = S_END;
        }

        // End when the input has ended and on stack is only $E
        if (isExpressionFinished(&stack) && input == S_END) {
            break;
        }

        switch (precedence_table[top_terminal.symbol][input]) {
        case '<':
            insertMark(&stack, top_terminal);

            // Case of function call
            // parse_function_call returns Node *, which is then saved to item
            if (current.type == TOKEN_ID && next.type == TOKEN_LPAREN) {
                StackItem item = {S_OPERAND, 0, parse_function_call()};
                pushStack(&stack, item);

                // Case for list literal
                // parse_list_literal returns Node *, which is then saved to item
            } else if (current.type == TOKEN_LIST && next.type == TOKEN_LPAREN) {
                StackItem item = {S_OPERAND, 0, parse_list_literal()};
                pushStack(&stack, item);

            } else if (current.type == TOKEN_LIST && next.type == TOKEN_DOT) {
                StackItem item = {S_OPERAND, 0, parse_list_range()};
                pushStack(&stack, item);

                // if in expression is if value, then we parse it and return the node as the operand
            } else if (current.type == TOKEN_IF) {
                StackItem item = {S_OPERAND, 0, parse_if_value()};
                pushStack(&stack, item);

                // Unit written as ()
            } else if (current.type == TOKEN_LPAREN && next.type == TOKEN_RPAREN) {
                StackItem item = {S_OPERAND, 0, ASTNewNode(NODE_UNIT, current.line)};
                pushStack(&stack, item);
                next_token();
                next_token();

                // Other cases
            } else {
                StackItem item = {input, current.type, NULL};

                // If the item is operand (variable, int, double, ...), then we have to create
                // leafNode
                if (input == S_OPERAND) {
                    item.node = LeafNode();
                }
                pushStack(&stack, item);
                next_token();
            }
            break;

        case '>':
            reduce(&stack);
            break;

        case '=': {
            StackItem item = {input, current.type, NULL};
            pushStack(&stack, item);
            next_token();
            break;
        }

        default:
            syntax_error("expression");
        }
    }

    Node *root = stack.items[1].node;
    freeStack(&stack);
    return root;
}
