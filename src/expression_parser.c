// @file expression_parser.c
// @brief Precedence parser for expressions
// @date 07-11-2026
//
// @author Lukas Sofka (xsofkal00)
// @author Email: xsofkal00@stud.fit.vutbr.cz
// Faculty of Information Technology, Brno University of Technology

#include "expression_parser.h"
#include "error.h"
#include "parser.h"
#include "scanner.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>


// clang-format off
typedef enum {
    S_OPERAND, // variable, double, int, string, ...
    S_LPAREN,  // (
    S_RPAREN,  // )
    S_DOT,     // .
    S_UNARY,   // -, !, +
    S_MUL,     // *, /
    S_ADD,     // +, -
    S_REL,     // <=, >=, >, <
    S_EQ,      // ==, !=
    S_AND,     // &&
    S_OR,      // ||
    S_RANGE,   // to
    S_END,     // $
    S_COUNT,   // Count of terminals
    S_EXPR,    // E
    S_MARK     // <
} Symbol;


typedef struct AStack {
    Symbol *items;
    int count;
    int capacity;
} Stack;


// @brief  Precedence Table
// row     - topmost terminal on the stack
// columnt - current input symbol
// <       - shift with mark
// =       - shift without mark
// >       - reduce
// ' '     - syntax error
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


// @brief Initialize the stack with capacity set with STACK_INITIAL_CAPACITY
void initStack(Stack *stack) {
    stack->capacity = STACK_INITIAL_CAPACITY;
    stack->count = 0;
    stack->items = malloc(sizeof(Symbol) * stack->capacity);
    if (!stack->items) {
        error_exit(ERR_INTERNAL, 0, "Memory allocation fail: initStack");
    }
}

// @brief Free stack from memory
void freeStack(Stack *stack) {
    free(stack->items);
}

// @brief Push an element to stack
void pushStack(Stack *stack, Symbol symbol) {
    if (stack->count == stack->capacity) {
        stack->capacity *= 2;
        Symbol *realloc_array = realloc(stack->items, stack->capacity * sizeof(Symbol));
        if (!realloc_array) {
            error_exit(ERR_INTERNAL, 0, "Memory allocation fail: pushStack");
        }
        stack->items = realloc_array;
    }

    stack->items[stack->count] = symbol;
    stack->count++;
}

// @brief Pop topmost element and return it
Symbol popStack(Stack *stack) {
    stack->count--;
    return stack->items[stack->count];
}

// @brief Predicate whether stack is empty
bool isEmptyStack(Stack *stack) {
    return stack->count == 0;
}

// @brief Returns the first terminal on the stack
// Skips S_MARK and S_EXPR
Symbol topTerminal(Stack *stack) {
    int i = stack->count - 1;
    while (stack->items[i] >= S_COUNT) {
        i--;
    }

    return stack->items[i];
}

// @brief Inserts S_MARK right after the given terminal
void insertMark(Stack *stack, Symbol terminal) {
    Stack temp;
    initStack(&temp);

    while (!isEmptyStack(stack) && stack->items[stack->count - 1] != terminal) {
        pushStack(&temp, popStack(stack));
    }

    pushStack(stack, S_MARK);

    while (!isEmptyStack(&temp)) {
        pushStack(stack, popStack(&temp));
    }

    freeStack(&temp);
}

// @brief Predicate whether the end condition is met
// End condition: $E
bool isExpressionFinished(Stack *stack) {
    return stack->count == 2 && stack->items[0] == S_END && stack->items[1] == S_EXPR;
}

// @brief Converts the current token to a symbol
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

    case TOKEN_PLUS:
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

    default:
        return S_END;
    }
}

// @brief Reduces right side of a rule to S_EXPR
// Left       Right Side
// E     ->   E op E
// E     ->   unary E
// E     ->   i
// E     ->   ( E )
void reduce(Stack *stack) {
    int mark_idx = stack->count - 1;
    while (stack->items[mark_idx] != S_MARK) {
        mark_idx--;
    }

    Symbol *handle = &stack->items[mark_idx + 1];
    int size = stack->count - mark_idx - 1;

    bool valid = false;
    switch (size) {
    case 1:
        if (handle[0] == S_OPERAND) {
            valid = true;
        }
        break;

    case 2:
        if (handle[0] == S_UNARY && handle[1] == S_EXPR) {
            valid = true;
        }
        break;

    case 3:
        // clang-format off
        if (handle[0] == S_LPAREN && handle[1] == S_EXPR && handle[2] == S_RPAREN) {
            valid = true;
        } else if (handle[0] == S_EXPR &&
                   (handle[1] == S_DOT || handle[1] == S_ADD || handle[1] == S_MUL ||
                    handle[1] == S_EQ  || handle[1] == S_REL || handle[1] == S_AND ||
                    handle[1] == S_OR  || handle[1] == S_RANGE) &&
                   handle[2] == S_EXPR) {
            valid = true;
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
    pushStack(stack, S_EXPR);
}

// @brief Predicate whether there is S_LPAREN on the stack
bool hasOpenParen(Stack *stack) {
    for (int i = 0; i < stack->count; i++) {
        if (stack->items[i] == S_LPAREN) {
            return true;
        }
    }

    return false;
}


// @brief Parses one expression using the precedence table
// Stops at the first token that is not part of an expression
void parse_expression(void) {
    Stack stack;
    initStack(&stack);
    pushStack(&stack, S_END);

    Symbol top_terminal, input;

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

        switch (precedence_table[top_terminal][input]) {
        case '<':
            insertMark(&stack, top_terminal);

            // TODO: maybe a bit loose definition of calling a function
            // Need to discsuc with others
            if (current.type == TOKEN_ID && next.type == TOKEN_LPAREN) {
                parse_function_call();
                pushStack(&stack, S_OPERAND);
            } else {
                pushStack(&stack, input);
                next_token();
            }
            break;

        case '>':
            reduce(&stack);
            break;

        case '=':
            pushStack(&stack, input);
            next_token();
            break;

        default:
            syntax_error("expression");
        }
    }

    freeStack(&stack);
}
