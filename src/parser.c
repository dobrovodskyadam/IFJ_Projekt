/// @file parser.c
/// @brief Syntax analysis by recursive descent
/// @date 23-10-2026
///
/// @author Lukas Sofka (xsofkal00)
/// @author Email: xsofkal00@stud.fit.vutbr.cz
/// Faculty of Information Technology, Brno University of Technology

#include "parser.h"
#include "ast.h"
#include "error.h"
#include "expression_parser.h"
#include "generator.h"
#include "scanner.h"
#include "semantic.h"
#include "symtable.h"
#include "tac_gen.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

ScannerData scanner;
Token previous, current, next;


/// @brief Fills in the first two tokens
void fill_tokens(void) {
    next_token();
    next_token();
}

/// @brief Shifts the tokens previous <- current <- next
void next_token(void) {
    previous = current;
    current = next;
    get_token(&next, &scanner);

    // Check for lexical error
    if (next.type == TOKEN_ERROR) {
        error_exit(ERR_LEXICAL, next.line, "lexical error");
    }
}

/// @brief Exits the compilation with a syntax error
/// @param expected tokens
void syntax_error(const char *expected) {
    if (previous.line == 0) {
        error_exit(ERR_SYNTAX, current.line, "expected %s, got %s", expected,
                   tokenTypeName(current.type));
    }

    error_exit(ERR_SYNTAX, current.line, "expected %s after %s, got %s", expected,
               tokenTypeName(previous.type), tokenTypeName(current.type));
}

/// @brief Checks whether the current token has the given type and calls next_token
void expect(TokenType type) {
    if (current.type != type) {
        syntax_error(tokenTypeName(type));
    }

    next_token();
}

/// @brief Expects TOKEN_ID and returns the pointer to its name
char *expect_id(void) {
    expect(TOKEN_ID);
    char *name = previous.value.str_val;
    previous.value.str_val = NULL;
    return name;
}

/// @brief Checks whether a token can start an expression
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

/// @brief <start> -> TOKEN_IMPORT TOKEN_IFJ TOKEN_EOL <list-def> TOKEN_EOF
Node *parse_start(void) {
    Node *ptr = ASTNewNode(NODE_PROGRAM, current.line);
    NodeListInit(&ptr->data.program_data.definitions);

    expect(TOKEN_IMPORT);
    expect(TOKEN_IFJ);
    expect(TOKEN_EOL);
    parse_list_def(&ptr->data.program_data.definitions);
    expect(TOKEN_EOF);

    return ptr;
}

/// @brief <list-def> -> <def> <list-def> | ε
void parse_list_def(NodeList *definitions) {
    if (current.type == TOKEN_MAIN || current.type == TOKEN_DEF) {
        NodeListAppend(definitions, parse_def());
        parse_list_def(definitions);
    } else if (current.type == TOKEN_EOF) {
        return;
    } else {
        syntax_error("TOKEN_MAIN, TOKEN_DEF or TOKEN_EOF");
    }
}

/// @brief <def> -> <opt-main> TOKEN_DEF TOKEN_ID TOKEN_LPAREN <list-params> TOKEN_RPAREN <opt-type>
///                 TOKEN_ASSIGN <function-body>
Node *parse_def(void) {
    Node *ptr = ASTNewNode(NODE_FUNCTION_DEF, current.line);

    ptr->data.function_def_data.is_main = parse_opt_main();
    expect(TOKEN_DEF);
    ptr->data.function_def_data.name = expect_id();
    expect(TOKEN_LPAREN);
    ptr->data.function_def_data.params = parse_list_params();
    expect(TOKEN_RPAREN);
    ptr->data.function_def_data.return_type = parse_opt_type();
    expect(TOKEN_ASSIGN);
    ptr->data.function_def_data.body = parse_function_body();

    return ptr;
}

/// @brief <opt-main> -> TOKEN_MAIN <opt-main-space> | ε
bool parse_opt_main(void) {
    switch (current.type) {
    case TOKEN_MAIN:
        expect(TOKEN_MAIN);
        parse_opt_main_space();
        return true;

    case TOKEN_DEF:
        return false;

    default:
        syntax_error("TOKEN_MAIN or TOKEN_DEF");
        return false;
    }
}

/// @brief <opt-main-space> -> TOKEN_EOL | ε
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

/// @brief <opt-type> -> TOKEN_COLON <type> | ε
Type parse_opt_type(void) {
    Type type;
    type.elem = NULL;
    type.kind = TYPE_UNDEF;

    switch (current.type) {
    case TOKEN_COLON:
        expect(TOKEN_COLON);
        type = parse_type();
        break;

    case TOKEN_ASSIGN:
        break;

    default:
        syntax_error("TOKEN_COLON or TOKEN_ASSIGN");
    }

    return type;
}

/// @brief <list-params> -> <parameter> <more-list-params> | ε
NodeList parse_list_params(void) {
    NodeList params;
    NodeListInit(&params);

    switch (current.type) {
    case TOKEN_ID:
        NodeListAppend(&params, parse_parameter());
        parse_more_list_params(&params);
        break;

    case TOKEN_RPAREN:
        break;

    default:
        syntax_error("TOKEN_ID or TOKEN_RPAREN");
    }

    return params;
}

/// @brief <parameter> -> TOKEN_ID TOKEN_COLON <type>
Node *parse_parameter(void) {
    Node *ptr = ASTNewNode(NODE_PARAMETER, current.line);

    ptr->data.param_data.name = expect_id();
    expect(TOKEN_COLON);
    ptr->data.param_data.type = parse_type();

    return ptr;
}

/// @brief <more-list-params-after> -> <parameter> <more-list-params> | ε
void parse_more_list_params_after(NodeList *params, unsigned int comma_line) {
    if (current.type == TOKEN_RPAREN) {
        if (current.line == comma_line)
            syntax_error("parameter after TOKEN_COMMA");
    } else if (current.type == TOKEN_ID) {
        NodeListAppend(params, parse_parameter());
        parse_more_list_params(params);
    } else {
        syntax_error("TOKEN_RPAREN or TOKEN_ID");
    }
}

/// @brief <more-list-params> -> TOKEN_COMMA <more-list-params-after> | ε
void parse_more_list_params(NodeList *params) {
    switch (current.type) {
    case TOKEN_COMMA: {
        unsigned int comma_line = current.line;
        expect(TOKEN_COMMA);
        parse_more_list_params_after(params, comma_line);
        break;
    }

    case TOKEN_RPAREN:
        break;

    default:
        syntax_error("TOKEN_COMMA or TOKEN_RPAREN");
    }
}

/// @brief <function-body> -> <if-or-expr> | TOKEN_EOL <block>
Node *parse_function_body(void) {
    if (current.type == TOKEN_EOL) {
        expect(TOKEN_EOL);
        return parse_block();
    } else if (current.type == TOKEN_IF || is_expression_start(current.type)) {
        return parse_if_or_expr();
    } else {
        syntax_error("TOKEN_EOL, expression or if");
        return NULL;
    }
}

/// @brief <block> -> TOKEN_INDENT <statement-list> TOKEN_DEDENT
Node *parse_block(void) {
    Node *ptr = ASTNewNode(NODE_BLOCK, current.line);
    NodeListInit(&ptr->data.block_data.statements);

    expect(TOKEN_INDENT);
    parse_statement_list(&ptr->data.block_data.statements);
    expect(TOKEN_DEDENT);

    return ptr;
}

/// @brief <if-or-expr> -> <expression> <statement-end> | <if-statement>
Node *parse_if_or_expr(void) {
    Node *ptr = NULL;

    if (current.type == TOKEN_IF) {
        ptr = parse_if_statement();
    } else if (is_expression_start(current.type)) {
        ptr = parse_expression();
        parse_statement_end();
    } else {
        syntax_error("expression or if statement");
    }

    return ptr;
}

/// @brief <type> -> TOKEN_UNIT
///                | TOKEN_KW_INT
///                | TOKEN_BOOLEAN
///                | TOKEN_KW_DOUBLE
///                | TOKEN_KW_STRING
///                | TOKEN_LIST TOKEN_LBRACKET <type> TOKEN_RBRACKET
Type parse_type(void) {
    Type type;
    type.kind = TYPE_UNKNOWN, type.elem = NULL;

    switch (current.type) {
    case TOKEN_UNIT:
        type.kind = TYPE_UNIT;
        next_token();
        break;

    case TOKEN_KW_INT:
        type.kind = TYPE_INT;
        next_token();
        break;

    case TOKEN_BOOLEAN:
        type.kind = TYPE_BOOL;
        next_token();
        break;

    case TOKEN_KW_DOUBLE:
        type.kind = TYPE_DOUBLE;
        next_token();
        break;

    case TOKEN_KW_STRING:
        type.kind = TYPE_STRING;
        next_token();
        break;

    case TOKEN_LIST:
        expect(TOKEN_LIST);
        expect(TOKEN_LBRACKET);

        type.kind = TYPE_LIST;
        type.elem = TypeInit(current.line);
        *type.elem = parse_type();

        expect(TOKEN_RBRACKET);
        break;

    default:
        syntax_error("type");
    }

    return type;
}

/// @brief <if-statement> -> TOKEN_IF <expression> TOKEN_THEN <st-then-part>
Node *parse_if_statement(void) {
    Node *ptr = ASTNewNode(NODE_IF_STATEMENT, current.line);
    expect(TOKEN_IF);
    ptr->data.if_data.condition = parse_expression();
    expect(TOKEN_THEN);
    parse_st_then_part(ptr);

    return ptr;
}

/// @brief <statement-list> -> <statement> <statement-list-rest>
void parse_statement_list(NodeList *statements) {
    NodeListAppend(statements, parse_statement());
    parse_statement_list_rest(statements);
}

/// @brief <statement-end> -> TOKEN_EOL | TOKEN_SEPARATOR
void parse_statement_end(void) {
    if (current.type == TOKEN_EOL) {
        next_token();
    } else if (current.type == TOKEN_SEPARATOR) {
        next_token();
        while (current.type == TOKEN_SEPARATOR) {
            next_token();
        }

        if (current.type == TOKEN_EOL) {
            next_token();
        }
    } else {
        syntax_error("end of line or ';'");
    }
}

/// @brief <st-then-part> -> TOKEN_EOL <block> <st-opt-else>
///                        | <expression> <st-after-inline-part>
///                        | TOKEN_RETURN <expression> <st-after-inline-part>
///                        | TOKEN_ID TOKEN_ASSIGN <expression> <st-after-inline-part>
void parse_st_then_part(Node *ptr) {
    if (current.type == TOKEN_EOL) {
        next_token();
        ptr->data.if_data.then_part = parse_block();
        ptr->data.if_data.else_part = parse_st_opt_else();

    } else if (current.type == TOKEN_RETURN) {
        Node *return_ptr = ASTNewNode(NODE_RETURN, current.line);
        expect(TOKEN_RETURN);
        return_ptr->data.return_data.value = parse_expression();
        ptr->data.if_data.then_part = return_ptr;
        ptr->data.if_data.else_part = parse_st_after_inline_part();

    } else if (current.type == TOKEN_ID && next.type == TOKEN_ASSIGN) {
        Node *assign = ASTNewNode(NODE_ASSIGN, current.line);
        assign->data.assign_data.name = expect_id();
        expect(TOKEN_ASSIGN);
        assign->data.assign_data.value = parse_expression();
        ptr->data.if_data.then_part = assign;
        ptr->data.if_data.else_part = parse_st_after_inline_part();

    } else if (is_expression_start(current.type)) {
        ptr->data.if_data.then_part = parse_expression();
        ptr->data.if_data.else_part = parse_st_after_inline_part();
    } else {
        syntax_error("TOKEN_EOL or expression");
    }
}

/// @brief <statement> -> <variable-declaration>
///                     | <assignment>
///                     | <return>
///                     | <while-statement>
///                     | <for-statement>
///                     | <if-statement>
///                     | <expression> <statement-end>
Node *parse_statement(void) {
    Node *ptr = NULL;

    switch (current.type) {
    case TOKEN_ID:
        if (next.type == TOKEN_ASSIGN) {
            ptr = parse_assignment();
        } else {
            ptr = parse_expression();
            parse_statement_end();
        }
        break;

    case TOKEN_IF:
        ptr = parse_if_statement();
        break;

    case TOKEN_WHILE:
        ptr = parse_while_statement();
        break;

    case TOKEN_FOR:
        ptr = parse_for_statement();
        break;

    case TOKEN_RETURN:
        ptr = parse_return();
        break;

    case TOKEN_VAL:
    case TOKEN_VAR:
        ptr = parse_variable_declaration();
        break;

    default:
        if (is_expression_start(current.type)) {
            ptr = parse_expression();
            parse_statement_end();
        } else {
            syntax_error("statement");
        }
    }

    return ptr;
}

/// @brief <statement-list-rest> -> <statement> <statement-list-rest> | ε
void parse_statement_list_rest(NodeList *statements) {
    if (current.type == TOKEN_DEDENT) {
        return;
    }

    NodeListAppend(statements, parse_statement());
    parse_statement_list_rest(statements);
}

/// @brief <st-opt-else> -> TOKEN_ELSE <st-else-part> | ε
Node *parse_st_opt_else(void) {
    if (current.type == TOKEN_ELSE) {
        expect(TOKEN_ELSE);
        return parse_st_else_part();
    }

    return NULL;
}

/// @brief <st-after-inline-part> -> TOKEN_ELSE <st-else-part>
///                                | TOKEN_EOL <st-opt-else>
///                                | TOKEN_SEPARATOR
Node *parse_st_after_inline_part(void) {
    Node *ptr = NULL;

    switch (current.type) {
    case TOKEN_ELSE:
        expect(TOKEN_ELSE);
        ptr = parse_st_else_part();
        break;

    case TOKEN_EOL:
        expect(TOKEN_EOL);
        ptr = parse_st_opt_else();
        break;

    case TOKEN_SEPARATOR:
        expect(TOKEN_SEPARATOR);
        break;

    default:
        syntax_error("TOKEN_ELSE, TOKEN_EOL or TOKEN_SEPARATOR");
    }

    return ptr;
}

/// @brief <variable-declaration> -> <var-or-val> TOKEN_ID <opt-type> TOKEN_ASSIGN <value-or-expr>
Node *parse_variable_declaration(void) {
    Node *ptr = ASTNewNode(NODE_VARIABLE_DECLARATION, current.line);

    ptr->data.var_decl_data.is_val = parse_var_or_val();
    ptr->data.var_decl_data.name = expect_id();
    ptr->data.var_decl_data.type = parse_opt_type();
    expect(TOKEN_ASSIGN);
    ptr->data.var_decl_data.value = parse_value_or_expr();

    return ptr;
}

/// @brief <assignment> -> TOKEN_ID TOKEN_ASSIGN <value-or-expr>
Node *parse_assignment(void) {
    Node *ptr = ASTNewNode(NODE_ASSIGN, current.line);
    ptr->data.assign_data.name = expect_id();
    expect(TOKEN_ASSIGN);
    ptr->data.assign_data.value = parse_value_or_expr();

    return ptr;
}

/// @brief <return> -> TOKEN_RETURN <return-argument>
Node *parse_return(void) {
    Node *ptr = ASTNewNode(NODE_RETURN, current.line);
    expect(TOKEN_RETURN);
    ptr->data.return_data.value = parse_return_argument();

    return ptr;
}

/// @brief <oneline-or-multiline> -> TOKEN_EOL <block> | <statement>
Node *parse_oneline_or_multiline() {
    switch (current.type) {
    case TOKEN_EOL:
        expect(TOKEN_EOL);
        return parse_block();

    default: {
        Node *ptr = ASTNewNode(NODE_BLOCK, current.line);
        NodeListInit(&ptr->data.block_data.statements);
        NodeListAppend(&ptr->data.block_data.statements, parse_statement());
        return ptr;
    }
    }
}

/// @brief <while-statement> -> TOKEN_WHILE <expression> TOKEN_DO <oneline-or-multiline>
Node *parse_while_statement(void) {
    Node *ptr = ASTNewNode(NODE_WHILE_CYCLE, current.line);

    expect(TOKEN_WHILE);
    ptr->data.while_data.condition = parse_expression();
    expect(TOKEN_DO);
    ptr->data.while_data.body = parse_oneline_or_multiline();

    return ptr;
}

/// @brief <for-statement> -> TOKEN_FOR <for-header> TOKEN_DO <oneline-or-multiline>
Node *parse_for_statement(void) {
    Node *ptr = ASTNewNode(NODE_FOR_CYCLE, current.line);

    expect(TOKEN_FOR);
    parse_for_header(ptr);
    expect(TOKEN_DO);
    ptr->data.for_data.body = parse_oneline_or_multiline();

    return ptr;
}

/// @brief <value-or-expr> -> <expression> <statement-end> | <if-value>
Node *parse_value_or_expr(void) {
    Node *ptr = NULL;

    switch (current.type) {
    case TOKEN_IF:
        ptr = parse_if_value();
        if (previous.type != TOKEN_DEDENT) {
            parse_statement_end();
        }
        break;
    default:
        if (is_expression_start(current.type)) {
            ptr = parse_expression();
            parse_statement_end();
        } else {
            syntax_error("TOKEN_IF or expression");
        }
    }

    return ptr;
}

/// @brief <var-or-val> -> TOKEN_VAL | TOKEN_VAR
bool parse_var_or_val(void) {
    switch (current.type) {
    case TOKEN_VAL:
        expect(TOKEN_VAL);
        return true;
    case TOKEN_VAR:
        expect(TOKEN_VAR);
        return false;
    default:
        syntax_error("TOKEN_VAL or TOKEN_VAR");
        return false;
    }
}

/// @brief <return-argument> -> <statement-end> | <value-or-expr>
Node *parse_return_argument(void) {
    switch (current.type) {
    case TOKEN_EOL:
    case TOKEN_SEPARATOR:
        parse_statement_end();
        return NULL;
    default:
        if (is_expression_start(current.type) || current.type == TOKEN_IF) {
            return parse_value_or_expr();
        } else {
            syntax_error("TOKEN_EOL, TOKEN_SEPARATOR or expression");
            return NULL;
        }
    }
}

/// @brief <guards-oneline> -> <guard> <guards-oneline> | ε
void parse_guards_oneline(NodeList *guards) {
    if (current.type == TOKEN_DO) {
        return;
    } else if (current.type == TOKEN_IF) {
        NodeListAppend(guards, parse_guard());
        parse_guards_oneline(guards);
    } else {
        syntax_error("TOKEN_IF or TOKEN_DO");
    }
}

/// @brief <guards-multiline> -> <guard> TOKEN_EOL <guards-multiline> | ε
void parse_guards_mutliline(NodeList *guards) {
    if (current.type == TOKEN_DEDENT) {
        return;
    } else if (current.type == TOKEN_IF) {
        NodeListAppend(guards, parse_guard());
        expect(TOKEN_EOL);
        parse_guards_mutliline(guards);
    } else {
        syntax_error("TOKEN_IF or TOKEN_DEDENT");
    }
}

/// @brief <for-header> -> <generator> <guards-oneline>
///                      | TOKEN_EOL TOKEN_INDENT <generator> TOKEN_EOL <guards-multiline>
///                        TOKEN_DEDENT
void parse_for_header(Node *ptr) {
    switch (current.type) {
    case TOKEN_ID:
        parse_generator(ptr);
        parse_guards_oneline(&ptr->data.for_data.guards);
        break;
    case TOKEN_EOL:
        expect(TOKEN_EOL);
        expect(TOKEN_INDENT);
        parse_generator(ptr);
        expect(TOKEN_EOL);

        parse_guards_mutliline(&ptr->data.for_data.guards);
        expect(TOKEN_DEDENT);
        break;
    default:
        syntax_error("TOKEN_EOL or TOKEN_ID");
    }
}

/// @brief <guard> -> TOKEN_IF <expression>
Node *parse_guard(void) {
    expect(TOKEN_IF);
    return parse_expression();
}

/// @brief <generator> -> TOKEN_ID TOKEN_GEN_ARROW <expression>
void parse_generator(Node *ptr) {
    ptr->data.for_data.name = expect_id();
    expect(TOKEN_GEN_ARROW);
    ptr->data.for_data.range = parse_expression();
}

/// @brief <st-else-part> -> TOKEN_EOL <block>
///                        | <expression> <statement-end>
///                        | <if-statement>
///                        | <return>
///                        | <assignment>
Node *parse_st_else_part(void) {
    Node *ptr = NULL;

    if (current.type == TOKEN_EOL) {
        expect(TOKEN_EOL);
        ptr = parse_block();
    } else if (current.type == TOKEN_IF) {
        ptr = parse_if_statement();
    } else if (current.type == TOKEN_RETURN) {
        ptr = parse_return();
    } else if (current.type == TOKEN_ID && next.type == TOKEN_ASSIGN) {
        ptr = parse_assignment();
    } else if (is_expression_start(current.type)) {
        ptr = parse_expression();
        parse_statement_end();
    } else {
        syntax_error("TOKEN_EOL or expression");
    }

    return ptr;
}

/// @brief <if-value> -> TOKEN_IF <expression> TOKEN_THEN <value-then-part>
Node *parse_if_value(void) {
    Node *ptr = ASTNewNode(NODE_IF_VALUE, current.line);
    expect(TOKEN_IF);
    ptr->data.if_data.condition = parse_expression();
    expect(TOKEN_THEN);
    parse_value_then_part(ptr);
    return ptr;
}

/// @brief <value-then-part> -> TOKEN_EOL <block> TOKEN_ELSE <value-else-part>
///                           | <expression> <value-after-inline-part>
///                           | <if-value> <value-after-inline-part>
void parse_value_then_part(Node *ptr) {
    if (current.type == TOKEN_EOL) {
        expect(TOKEN_EOL);
        ptr->data.if_data.then_part = parse_block();
        expect(TOKEN_ELSE);
        ptr->data.if_data.else_part = parse_value_else_part();
    } else if (current.type == TOKEN_IF) {
        ptr->data.if_data.then_part = parse_expression();
        ptr->data.if_data.else_part = parse_value_after_inline_part();
    } else if (is_expression_start(current.type)) {
        ptr->data.if_data.then_part = parse_expression();
        ptr->data.if_data.else_part = parse_value_after_inline_part();
    } else {
        syntax_error("TOKEN_EOL or expression");
    }
}


/// @brief <value-after-inline-part> -> TOKEN_ELSE <value-else-part>
///                                   | TOKEN_EOL TOKEN_ELSE <value-else-part>
Node *parse_value_after_inline_part(void) {
    Node *ptr = NULL;

    switch (current.type) {
    case TOKEN_ELSE:
        expect(TOKEN_ELSE);
        ptr = parse_value_else_part();
        break;

    case TOKEN_EOL:
        expect(TOKEN_EOL);
        expect(TOKEN_ELSE);
        ptr = parse_value_else_part();
        break;

    default:
        syntax_error("TOKEN_ELSE or TOKEN_EOL");
    }

    return ptr;
}

/// @brief <value-else-part> -> <expression> | TOKEN_EOL <block> | <if-value>
Node *parse_value_else_part(void) {
    Node *ptr = NULL;

    if (current.type == TOKEN_EOL) {
        expect(TOKEN_EOL);
        ptr = parse_block();
    } else if (current.type == TOKEN_IF) {
        ptr = parse_if_value();
    } else if (is_expression_start(current.type)) {
        ptr = parse_expression();
    } else {
        syntax_error("TOKEN_EOL or expression");
    }

    return ptr;
}

/// @brief <function-call> -> TOKEN_ID TOKEN_LPAREN <list-arguments> TOKEN_RPAREN
Node *parse_function_call(void) {
    Node *ptr = ASTNewNode(NODE_FUNCTION_CALL, current.line);
    ptr->data.call_data.name = expect_id();
    expect(TOKEN_LPAREN);
    parse_list_arguments(&ptr->data.call_data.arguments);
    expect(TOKEN_RPAREN);

    return ptr;
}

/// @brief <list-arguments> -> <argument> <more-list-arguments> | ε
void parse_list_arguments(NodeList *arguments) {
    if (current.type == TOKEN_RPAREN) {
        return;

    } else if (is_expression_start(current.type) || current.type == TOKEN_IF) {
        NodeListAppend(arguments, parse_argument());
        parse_more_list_arguments(arguments);
    } else {
        syntax_error("TOKEN_RPAREN or expression");
    }
}

/// @brief <more-list-arguments-after> -> <argument> <more-list-arguments> | ε
void parse_more_list_arguments_afrer(NodeList *arguments, unsigned int comma_line) {
    if (current.type == TOKEN_RPAREN) {
        if (comma_line == current.line) {
            syntax_error("RPAREN must start on new line");
        }
    } else if (is_expression_start(current.type)) {
        NodeListAppend(arguments, parse_argument());
        parse_more_list_arguments(arguments);

    } else {
        syntax_error("TOKEN_RPAREN or expression");
    }
}

/// @brief <more-list-arguments> -> TOKEN_COMMA <more-list-arguments-after> | ε
void parse_more_list_arguments(NodeList *arguments) {
    switch (current.type) {
    case TOKEN_RPAREN:
        break;
    case TOKEN_COMMA: {
        unsigned int comma_line = current.line;
        expect(TOKEN_COMMA);
        parse_more_list_arguments_afrer(arguments, comma_line);
        break;
    }
    default:
        syntax_error("TOKEN_RPAREN or TOKEN_COMMA");
    }
}

/// @brief <argument> -> <expression>
Node *parse_argument(void) {
    return parse_expression();
}

/// @brief <list-literal> -> TOKEN_LIST TOKEN_LPAREN <list-arguments> TOKEN_RPAREN
Node *parse_list_literal(void) {
    Node *ptr = ASTNewNode(NODE_LIST_LITERAL, current.line);
    expect(TOKEN_LIST);
    expect(TOKEN_LPAREN);
    parse_list_arguments(&ptr->data.list_data.items);
    expect(TOKEN_RPAREN);

    return ptr;
}

/// @brief Static function parser
Node *parse_list_range(void) {
    Node *ptr = ASTNewNode(NODE_FUNCTION_CALL, current.line);
    expect(TOKEN_LIST);
    expect(TOKEN_DOT);

    char *id = expect_id();
    char *name = malloc(strlen("List.") + strlen(id) + 1);
    if (!name) {
        error_exit(ERR_INTERNAL, 0, "Memory allocation fail: parse_list_range");
    }
    strcpy(name, "List.");
    strcat(name, id);
    free(id);

    ptr->data.call_data.name = name;

    expect(TOKEN_LPAREN);
    parse_list_arguments(&ptr->data.call_data.arguments);
    if (ptr->data.call_data.arguments.count == 2) {
        Node *step = ASTNewNode(NODE_INT, current.line);
        step->data.int_value = 1;
        NodeListAppend(&ptr->data.call_data.arguments, step);
    }
    expect(TOKEN_RPAREN);
    return ptr;
}

/// @brief Init the scanner and do the syntax analysis
/// @return Root of the AST
Node *parse(void) {
    fill_tokens();
    return parse_start();
}

/// @brief Main function of the whole compiler
int main(void) {
    init_scanner_data(&scanner);
    Node *root = parse();
    free_scanner_data(&scanner);

#ifdef DEBUG
    ASTprint(root, 0);
#endif

    semantic_analyze(root);

    TacList tac;
    tac_list_init(&tac);

    tac_gen_statement(root, &tac);

    generate_ifjcode26(&tac, stdout);

    tac_list_free(&tac);

    ASTFree(root);
    TypeFreeAll();
    return 0;
}
