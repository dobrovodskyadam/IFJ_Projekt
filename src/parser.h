/// @file parser.h
/// @brief
/// @date 23-10-2026
///
/// @author Lukas Sofka (xsofkal00)
/// @author Email: xsofkal00@stud.fit.vutbr.cz
/// Faculty of Information Technology, Brno University of Technology`

#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "scanner.h"
#include <stdbool.h>

// Declarations
// Nonterminal parsers

// Program structure
Node *parse_start(void);
void parse_list_def(NodeList *definitions);

// Function definition
Node *parse_def(void);
bool parse_opt_main(void);
void parse_opt_main_space(void);

// Parameters
NodeList parse_list_params(void);
void parse_more_list_params(NodeList *params);
Node *parse_parameter(void);

// Function body
Node *parse_function_body(void);

// Types
Type parse_type(void);
Type parse_opt_type(void);

// Blocks
Node *parse_block(void);
void parse_statement_list(NodeList *statements);
void parse_statement_list_rest(NodeList *statements);

// Statements
Node *parse_statement(void);
void parse_statement_end(void);
Node *parse_if_or_expr(void);
Node *parse_value_or_expr(void);

// Var/Val Declaration
Node *parse_variable_declaration(void);
bool parse_var_or_val(void);

// Assignment
Node *parse_assignment(void);

// Return
Node *parse_return(void);
Node *parse_return_argument(void);

// While cycle
Node *parse_while_statement(void);

// For cycle
Node *parse_for_statement(void);
void parse_for_header(Node *ptr);
void parse_list_guards(NodeList *guards);
Node *parse_guard(void);
void parse_generator(Node *ptr);

// If Statement
Node *parse_if_statement(void);
void parse_st_then_part(Node *ptr);
Node *parse_st_else_part(void);
Node *parse_st_opt_else(void);
Node *parse_st_after_inline_part(void);

// If Value
Node *parse_if_value(void);
void parse_value_then_part(Node *ptr);
Node *parse_value_after_inline_part(void);
Node *parse_value_else_part(void);

// Function Call
Node *parse_function_call(void);

// Function Arguments
void parse_list_arguments(NodeList *arguments);
void parse_more_list_arguments(NodeList *arguments);
Node *parse_argument(void);

// List Literal
Node *parse_list_literal(void);

// Helper functions declarations
void fill_tokens(void);
void next_token(void);
void expect(TokenType type);
char *expect_id(void);
void syntax_error(const char *expected);
bool is_expression_start(TokenType type);
Node *parse(void);

#endif
