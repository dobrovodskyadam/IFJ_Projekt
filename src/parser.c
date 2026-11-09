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
#include "expression_parser.h"
#include "scanner.h"
#include "symtable.h"

#include <stdbool.h>
#include <stdio.h>

ScannerData scanner;
Token previous, current, next;


// @brief
void fill_tokens(void) {
    next_token();
    next_token();
}

// @brief
void next_token(void) {
    previous = current;
    current = next;
    getToken(&next, &scanner);

    // Check for lexical error
    if (next.type == TOKEN_ERROR) {
        error_exit(ERR_LEXICAL, next.line, "lexical error");
    }
}

// @brief
void syntax_error(const char *expected) {
    if (previous.line == 0) {
        error_exit(ERR_SYNTAX, current.line, "Expected %s, got %s", expected,
                   tokenTypeName(current.type));
    }

    error_exit(ERR_SYNTAX, current.line, "Expected %s after %s, got %s", expected,
               tokenTypeName(previous.type), tokenTypeName(current.type));
}


// @brief
void expect(TokenType type) {
    if (current.type != type) {
        syntax_error(tokenTypeName(type));
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
        syntax_error("TOKEN_MAIN, TOKEN_DEF or TOKEN_EOF");
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
        syntax_error("TOKEN_MAIN or TOKEN_DEF");
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
        syntax_error("TOKEN_EOL or TOKEN_DEF");
    }
}

// @brief
void parse_opt_type(void) {
    switch (current.type) {
    case TOKEN_COLON:
        expect(TOKEN_COLON);
        parse_type();
        break;

    case TOKEN_ASSIGN:
        break;

    default:
        syntax_error("TOKEN_COLON or TOKEN_ASSIGN");
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
        syntax_error("TOKEN_ID or TOKEN_RPAREN");
    }
}

// @brief
void parse_parameter(void) {
    expect(TOKEN_ID);
    expect(TOKEN_COLON);
    parse_type();
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
        syntax_error("TOKEN_COMMA or TOKEN_RPAREN");
    }
}

void parse_function_body(void) {
    if (current.type == TOKEN_EOL) {
        expect(TOKEN_EOL);
        parse_block();
    } else if (current.type == TOKEN_IF || is_expression_start(current.type)) {
        parse_if_or_expr();
    } else {
        syntax_error("TOKEN_EOL, expression or if");
    }
}

// @brief
void parse_block(void) {
    expect(TOKEN_INDENT);
    parse_statement_list();
    expect(TOKEN_DEDENT);
}

// @brief
void parse_if_or_expr(void) {
    if (current.type == TOKEN_IF) {
        parse_if_statement();
    } else if (is_expression_start(current.type)) {
        parse_expression();
        parse_statement_end();
    } else {
        syntax_error("expression or if statement");
    }
}

// @brief
void parse_type(void) {
    switch (current.type) {
    case TOKEN_UNIT:
    case TOKEN_KW_INT:
    case TOKEN_BOOLEAN:
    case TOKEN_KW_DOUBLE:
    case TOKEN_KW_STRING:
        next_token();
        break;

    case TOKEN_LIST:
        expect(TOKEN_LIST);
        expect(TOKEN_LBRACKET);
        parse_type();
        expect(TOKEN_RBRACKET);
        break;

    default:
        syntax_error("type");
    }
}

// @brief
void parse_if_statement(void) {
    expect(TOKEN_IF);
    parse_expression();
    expect(TOKEN_THEN);
    parse_st_then_part();
}

// @brief
void parse_statement_list(void) {
    parse_statement();
    parse_statement_list_rest();
}

// @brief
void parse_statement_end(void) {
    if (current.type == TOKEN_EOL || current.type == TOKEN_SEPARATOR) {
        next_token();
    } else {
        syntax_error("end of line or ';'");
    }
}

// @brief
void parse_st_then_part(void) {
    if (current.type == TOKEN_EOL) {
        next_token();
        parse_block();
        parse_st_opt_else();
    } else if (is_expression_start(current.type)) {
        parse_expression();
        parse_st_after_inline_part();
    } else {
        syntax_error("TOKEN_EOL or expression");
    }
}

// @brief
void parse_statement(void) {
    switch (current.type) {
    case TOKEN_ID:
        if (next.type == TOKEN_ASSIGN) {
            parse_assignment();
        } else {
            parse_expression();
            parse_statement_end();
        }
        break;

    case TOKEN_IF:
        parse_if_statement();
        break;

    case TOKEN_WHILE:
        parse_while_statement();
        break;

    case TOKEN_FOR:
        parse_for_statement();
        break;

    case TOKEN_RETURN:
        parse_return();
        break;

    case TOKEN_VAL:
    case TOKEN_VAR:
        parse_variable_declaration();
        break;

    default:
        if (is_expression_start(current.type)) {
            parse_expression();
            parse_statement_end();
        } else {
            syntax_error("statement");
        }
    }
}

// @brief
void parse_statement_list_rest(void) {
    if (current.type == TOKEN_DEDENT) {
        return;
    }

    parse_statement();
    parse_statement_list_rest();
}

// @brief
void parse_st_opt_else(void) {
    if (current.type == TOKEN_ELSE) {
        expect(TOKEN_ELSE);
        parse_st_else_part();
    }
}

// @brief
void parse_st_after_inline_part(void) {
    switch (current.type) {
    case TOKEN_ELSE:
        expect(TOKEN_ELSE);
        parse_st_else_part();
        break;

    case TOKEN_EOL:
        expect(TOKEN_EOL);
        parse_st_opt_else();
        break;

    case TOKEN_SEPARATOR:
        expect(TOKEN_SEPARATOR);
        break;

    default:
        syntax_error("TOKEN_ELSE, TOKEN_EOL or TOKEN_SEPARATOR");
    }
}

// @brief
void parse_variable_declaration(void) {
    parse_var_or_val();
    expect(TOKEN_ID);
    parse_opt_type();
    expect(TOKEN_ASSIGN);
    parse_value_or_expr();
}

// @brief
void parse_assignment(void) {
    expect(TOKEN_ID);
    expect(TOKEN_ASSIGN);
    parse_value_or_expr();
}

// @brief
void parse_return(void) {
    expect(TOKEN_RETURN);
    parse_return_argument();
}

// @brief
void parse_while_statement(void) {
    expect(TOKEN_WHILE);
    parse_expression();
    expect(TOKEN_DO);
    expect(TOKEN_EOL);
    parse_block();
}

// @brief
void parse_for_statement(void) {
    expect(TOKEN_FOR);
    parse_for_header();
    expect(TOKEN_DO);
    expect(TOKEN_EOL);
    parse_block();
}

// @brief
void parse_value_or_expr(void) {
    switch (current.type) {
    case TOKEN_IF:
        parse_if_value();
        break;
    default:
        if (is_expression_start(current.type)) {
            parse_expression();
            parse_statement_end();
        } else {
            syntax_error("TOKEN_IF or expression");
        }
    }
}

// @brief
void parse_var_or_val(void) {
    switch (current.type) {
    case TOKEN_VAL:
        expect(TOKEN_VAL);
        break;
    case TOKEN_VAR:
        expect(TOKEN_VAR);
        break;
    default:
        syntax_error("TOKEN_VAL or TOKEN_VAR");
    }
}

// @brief
void parse_return_argument(void) {
    switch (current.type) {
    case TOKEN_EOL:
    case TOKEN_SEPARATOR:
        parse_statement_end();
        break;
    default:
        if (is_expression_start(current.type) || current.type == TOKEN_IF) {
            parse_value_or_expr();
        } else {
            syntax_error("TOKEN_EOL, TOKEN_SEPARATOR or expression");
        }
    }
}

// @brief
void parse_for_header(void) {
    switch (current.type) {
    case TOKEN_ID:
        parse_generator();
        break;
    case TOKEN_EOL:
        expect(TOKEN_EOL);
        expect(TOKEN_INDENT);
        parse_generator();
        expect(TOKEN_EOL);
        parse_list_guards();
        expect(TOKEN_DEDENT);
        break;
    default:
        syntax_error("TOKEN_EOL or TOKEN_ID");
    }
}

// @brief
void parse_list_guards(void) {
    switch (current.type) {
    case TOKEN_IF:
        parse_guard();
        expect(TOKEN_EOL);
        parse_list_guards();
        break;
    case TOKEN_DEDENT:
        break;
    default:
        syntax_error("");
    }
}

// @brief
void parse_guard(void) {
    expect(TOKEN_IF);
    parse_expression();
}

// @brief
void parse_generator(void) {
    expect(TOKEN_ID);
    expect(TOKEN_GEN_ARROW);
    parse_expression();
}

// @brief
void parse_st_else_part(void) {
    if (current.type == TOKEN_EOL) {
        expect(TOKEN_EOL);
        parse_block();
    } else if (current.type == TOKEN_IF) {
        parse_if_statement();
    } else if (is_expression_start(current.type)) {
        parse_expression();
        parse_statement_end();
    } else {
        syntax_error("TOKEN_EOL or expression");
    }
}

// @brief
void parse_if_value(void) {
    expect(TOKEN_IF);
    parse_expression();
    expect(TOKEN_THEN);
    parse_value_then_part();
}

// @brief
void parse_value_then_part(void) {
    if (current.type == TOKEN_EOL) {
        parse_block();
    } else if (is_expression_start(current.type)) {
        parse_expression();
        parse_value_then_part();
    } else {
        syntax_error("TOKEN_EOL or expression");
    }
}

// @brief
void parse_value_after_inline_part(void) {
    switch (current.type) {
    case TOKEN_ELSE:
        expect(TOKEN_ELSE);
        parse_value_else_part();
        break;
    case TOKEN_EOL:
        expect(TOKEN_EOL);
        expect(TOKEN_ELSE);
        parse_value_else_part();
        break;
    default:
        syntax_error("TOKEN_ELSE or TOKEN_EOL");
    }
}

// @brief
void parse_value_else_part(void) {
    if (current.type == TOKEN_EOL) {
        expect(TOKEN_EOL);
        parse_block();
    } else if (is_expression_start(current.type)) {
        parse_expression();
        parse_statement_end();
    } else {
        syntax_error("TOKEN_EOL or expression");
    }
}

// @brief
void parse_function_call(void) {
    expect(TOKEN_ID);
    expect(TOKEN_LPAREN);
    parse_list_arguments();
    expect(TOKEN_RPAREN);
}

// @brief
void parse_list_arguments(void) {
    if (current.type == TOKEN_RPAREN) {
        return;

    } else if (is_expression_start(current.type)) {
        parse_argument();
        parse_more_list_arguments();
    } else {
        syntax_error("TOKEN_RPAREN or expression");
    }
}

// @brief
void parse_more_list_arguments(void) {
    switch (current.type) {
    case TOKEN_RPAREN:
        break;
    case TOKEN_COMMA:
        expect(TOKEN_COMMA);
        parse_argument();
        parse_more_list_arguments();
        break;
    default:
        syntax_error("TOKEN_RPAREN or TOKEN_COMMA");
    }
}

// @brief
void parse_argument(void) {
    parse_expression();
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
        fprintf(stderr, "Syntax is okay\n");
    }
}
