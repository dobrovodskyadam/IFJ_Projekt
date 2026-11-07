// @file parser.c
// @brief
// @date 23-10-2026
//
// @author Lukas Sofka (xsofkal00)
// @author Email: xsofkal00@stud.fit.vutbr.cz
// Faculty of Information Technology, Brno University of Technology

// TODO: Add brief desriptions to the functions

#include "parser.h"
#include "error.h"
#include "scanner.h"
#include "symtable.h"

#include <stdbool.h>
#include <stdio.h>

ScannerData scanner;
Token next, current;

// Declarations
// Nonterminal parsers
void parse_start(void);
void parse_def(void);
void parse_list_def(void);
void parse_opt_main(void);
void parse_opt_main_space(void);
void parse_list_params(void);
void parse_parameter(void);
void parse_more_list_params(void);
void parse_function_body(void);
void parse_opt_type(void);
void parse_block(void);
void parse_if_or_expr(void);
// TODO: Add the rest of the rules

// Helper functions declarations
void fill_tokens(void);
void next_token(void);
void expect(TokenType type);
int parse(void);


// @brief
void fill_tokens(void) {
    next_token();
    next_token();
}

// @brief
void next_token(void) {
    current = next;
    getToken(&next, &scanner);

    // Check for lexical error
    if (next.type == TOKEN_ERROR) {
        error_exit(ERR_LEXICAL, "lexical error");
    }
}

// @brief
void expect(TokenType type) {
    if (current.type != type) {
        error_exit(ERR_SYNTAX, "Expected %s, got %s", tokenTypeName(type),
                   tokenTypeName(current.type));
    }

    next_token();
}

// @brief
bool is_expression_start(TokenType type) {
    switch (type) {
    case TOKEN_ID:
    case TOKEN_INT:
    case TOKEN_DOUBLE:
    case TOKEN_STRING:
    case TOKEN_TRUE:
    case TOKEN_FALSE:
    case TOKEN_LPAREN:
    case TOKEN_MINUS:
    case TOKEN_NOT:
    case TOKEN_LIST:
        return true;
    default:
        return false;
    }
}

// @brief Parse each nonterminal
void parse_start(void) {
    expect(TOKEN_IMPORT);
    expect(TOKEN_IFJ);
    expect(TOKEN_EOL);
    parse_list_def();
    expect(TOKEN_EOF);
}

// @brief
void parse_list_def(void) {
    if (current.type == TOKEN_MAIN || current.type == TOKEN_DEF) {
        parse_def();
        parse_list_def();
    } else if (current.type == TOKEN_EOF) {
        return;
    } else {
        error_exit(ERR_SYNTAX, "Received unexpected token %s", tokenTypeName(current.type));
    }
}

// @brief
void parse_def(void) {
    parse_opt_main();
    expect(TOKEN_DEF);
    expect(TOKEN_ID);
    expect(TOKEN_LPAREN);
    parse_list_params();
    expect(TOKEN_RPAREN);
    parse_opt_type();
    expect(TOKEN_ASSIGN);
    parse_function_body();
    expect(TOKEN_EOL);
}

// @brief
void parse_opt_main(void) {
    switch (current.type) {
    case TOKEN_MAIN:
        expect(TOKEN_MAIN);
        parse_opt_main_space();
        break;

    case TOKEN_DEF:
        break;

    default:
        error_exit(ERR_SYNTAX, "Expected TOKEN_MAIN or TOKEN_DEF, got %s",
                   tokenTypeName(current.type));
    }
}

// @brief
void parse_opt_main_space(void) {
    switch (current.type) {
    case TOKEN_EOL:
        expect(TOKEN_EOL);
        break;

    case TOKEN_DEF:
        break;

    default:
        error_exit(ERR_SYNTAX, "Expected TOKEN_EOL or TOKEN_DEF, got %s",
                   tokenTypeName(current.type));
    }
}

// @brief
void parse_opt_type(void) {
    switch (current.type) {
    case TOKEN_COLON:
        expect(TOKEN_COLON);
        break;

    case TOKEN_ASSIGN:
        break;

    default:
        error_exit(ERR_SYNTAX, "Expected TOKEN_COLON or TOKEN_ASSIGN, got %s",
                   tokenTypeName(current.type));
    }
}

// @brief
void parse_list_params(void) {
    switch (current.type) {
    case TOKEN_ID:
        parse_parameter();
        parse_more_list_params();
        break;

    case TOKEN_RPAREN:
        break;

    default:
        error_exit(ERR_SYNTAX, "Expected TOKEN_ID or TOKEN_RPAREN, got %s",
                   tokenTypeName(current.type));
    }
}

// @brief
void parse_parameter(void) {
    expect(TOKEN_ID);
    expect(TOKEN_COLON);
    // parse_type();
}

// @brief
void parse_more_list_params(void) {
    switch (current.type) {
    case TOKEN_COMMA:
        expect(TOKEN_COMMA);
        parse_parameter();
        parse_more_list_params();
        break;

    case TOKEN_RPAREN:
        break;

    default:
        error_exit(ERR_SYNTAX, "Expected TOKEN_COMMA or TOKEN_RPAREN, got %s",
                   tokenTypeName(current.type));
    }
}

void parse_function_body(void) {
    if (current.type == TOKEN_EOL) {
        expect(TOKEN_EOL);
        // parse_block();
    } else if (current.type == TOKEN_IF || is_expression_start(current.type)) {
        // parse_if_or_expr();
    } else {
        error_exit(ERR_SYNTAX, "Expected TOKEN_EOL, expression or if, got %s",
                   tokenTypeName(current.type));
    }
}

// @brief
int parse(void) {
    initScannerData(&scanner);
    fill_tokens();
    parse_start();
    return 0;
}

// @brief
int main(void) {
    if (parse() == 0) {
        fprintf(stderr, "Syntax is good\n");
    }
}
