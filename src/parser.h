// @file parser.h
// @brief
// @date 23-10-2026
//
// @author Lukas Sofka (xsofkal00)
// @author Email: xsofkal00@stud.fit.vutbr.cz
// Faculty of Information Technology, Brno University of Technology`

#ifndef PARSER_H
#define PARSER_H

#include "scanner.h"
#include <stdbool.h>

// Declarations
// Nonterminal parsers

// Program structure
void parse_start(void);
void parse_list_def(void);

// Function definition
void parse_def(void);
void parse_opt_main(void);
void parse_opt_main_space(void);

// Parameters
void parse_list_params(void);
void parse_more_list_params(void);
void parse_parameter(void);

// Function body
void parse_function_body(void);

// Types
void parse_type(void);
void parse_opt_type(void);

// Blocks
void parse_block(void);
void parse_statement_list(void);
void parse_statement_list_rest(void);

// Statements
void parse_statement(void);
void parse_statement_end(void);
void parse_if_or_expr(void);
void parse_value_or_expr(void);

// Var/Val Declaration
void parse_variable_declaration(void);
void parse_var_or_val(void);

// Assignment
void parse_assignment(void);

// Return
void parse_return(void);
void parse_return_argument(void);

// While cycle
void parse_while_statement(void);

// For cycle
void parse_for_statement(void);
void parse_for_header(void);
void parse_list_guards(void);
void parse_guard(void);
void parse_generator(void);

// If Statement
void parse_if_statement(void);
void parse_st_then_part(void);
void parse_st_else_part(void);
void parse_st_opt_else(void);
void parse_st_after_inline_part(void);

// If Value
void parse_if_value(void);
void parse_value_then_part(void);
void parse_value_after_inline_part(void);
void parse_value_else_part(void);

// Expressions (parsed elsewhere)
void parse_expression(void);

// Function Call
void parse_function_call(void);

// Function Arguments
void parse_list_arguments(void);
void parse_more_list_arguments(void);
void parse_argument(void);

// List Literal
void parse_list_literal(void);

// Helper functions declarations
void fill_tokens(void);
void next_token(void);
void expect(TokenType type);
void syntax_error(const char *expected);
bool is_expression_start(TokenType type);
int parse(void);

#endif
