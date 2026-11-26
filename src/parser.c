/// @file parser.c
/// @brief
/// @date 23-10-2026
///
/// @author Lukas Sofka (xsofkal00)
/// @author Email: xsofkal00@stud.fit.vutbr.cz
/// Faculty of Information Technology, Brno University of Technology

// TODO: Add brief desriptions to the functions

#include "parser.h"
#include "ast.h"
#include "error.h"
#include "expression_parser.h"
#include "scanner.h"
#include "semantic.h"
#include "symtable.h"

#include <stdbool.h>
#include <stdlib.h>

ScannerData scanner;
Token previous, current, next;


/// @brief
void fill_tokens(void) {
    next_token();
    next_token();
}

/// @brief
void next_token(void) {
    previous = current;
    current = next;
    getToken(&next, &scanner);

    // Check for lexical error
    if (next.type == TOKEN_ERROR) {
        error_exit(ERR_LEXICAL, next.line, "lexical error");
    }
}

/// @brief
void syntax_error(const char *expected) {
    if (previous.line == 0) {
        error_exit(ERR_SYNTAX, current.line, "expected %s, got %s", expected,
                   tokenTypeName(current.type));
    }

    error_exit(ERR_SYNTAX, current.line, "expected %s after %s, got %s", expected,
               tokenTypeName(previous.type), tokenTypeName(current.type));
}


/// @brief
void expect(TokenType type) {
    if (current.type != type) {
        syntax_error(tokenTypeName(type));
    }

    next_token();
}

char *expect_id(void) {
    expect(TOKEN_ID);
    char *name = previous.value.str_val;
    previous.value.str_val = NULL;
    return name;
}

/// @brief
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

/// @brief Parse each nonterminal
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

/// @brief
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

/// @brief
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

/// @brief
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

/// @brief
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

/// @brief
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

/// @brief
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

/// @brief
Node *parse_parameter(void) {
    Node *ptr = ASTNewNode(NODE_PARAMETER, current.line);

    ptr->data.param_data.name = expect_id();
    expect(TOKEN_COLON);
    ptr->data.param_data.type = parse_type();

    return ptr;
}

/// @brief
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

/// @brief
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

/// @brief
Node *parse_block(void) {
    Node *ptr = ASTNewNode(NODE_BLOCK, current.line);
    NodeListInit(&ptr->data.block_data.statements);

    expect(TOKEN_INDENT);
    parse_statement_list(&ptr->data.block_data.statements);
    expect(TOKEN_DEDENT);

    return ptr;
}

/// @brief
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

/// @brief
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

/// @brief
Node *parse_if_statement(void) {
    Node *ptr = ASTNewNode(NODE_IF_STATEMENT, current.line);
    expect(TOKEN_IF);
    ptr->data.if_data.condition = parse_expression();
    expect(TOKEN_THEN);
    parse_st_then_part(ptr);

    return ptr;
}

/// @brief
void parse_statement_list(NodeList *statements) {
    NodeListAppend(statements, parse_statement());
    parse_statement_list_rest(statements);
}

/// TODO
/// @brief
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

/// @brief
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

    } else if (is_expression_start(current.type)) {
        ptr->data.if_data.then_part = parse_expression();
        ptr->data.if_data.else_part = parse_st_after_inline_part();
    } else {
        syntax_error("TOKEN_EOL or expression");
    }
}

/// @brief
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

/// @brief
void parse_statement_list_rest(NodeList *statements) {
    if (current.type == TOKEN_DEDENT) {
        return;
    }

    NodeListAppend(statements, parse_statement());
    parse_statement_list_rest(statements);
}

/// @brief
Node *parse_st_opt_else(void) {
    if (current.type == TOKEN_ELSE) {
        expect(TOKEN_ELSE);
        return parse_st_else_part();
    }

    return NULL;
}

/// @brief
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

/// @brief
Node *parse_variable_declaration(void) {
    Node *ptr = ASTNewNode(NODE_VARIABLE_DECLARATION, current.line);

    ptr->data.var_decl_data.is_val = parse_var_or_val();
    ptr->data.var_decl_data.name = expect_id();
    ptr->data.var_decl_data.type = parse_opt_type();
    expect(TOKEN_ASSIGN);
    ptr->data.var_decl_data.value = parse_value_or_expr();

    return ptr;
}

/// @brief
Node *parse_assignment(void) {
    Node *ptr = ASTNewNode(NODE_ASSIGN, current.line);
    ptr->data.assign_data.name = expect_id();
    expect(TOKEN_ASSIGN);
    ptr->data.assign_data.value = parse_value_or_expr();

    return ptr;
}

/// @brief
Node *parse_return(void) {
    Node *ptr = ASTNewNode(NODE_RETURN, current.line);
    expect(TOKEN_RETURN);
    ptr->data.return_data.value = parse_return_argument();

    return ptr;
}

/// @brief
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

/// @brief
Node *parse_while_statement(void) {
    Node *ptr = ASTNewNode(NODE_WHILE_CYCLE, current.line);

    expect(TOKEN_WHILE);
    ptr->data.while_data.condition = parse_expression();
    expect(TOKEN_DO);
    expect(TOKEN_EOL);
    ptr->data.while_data.body = parse_oneline_or_multiline();

    return ptr;
}

/// @brief
Node *parse_for_statement(void) {
    Node *ptr = ASTNewNode(NODE_FOR_CYCLE, current.line);

    expect(TOKEN_FOR);
    parse_for_header(ptr);
    expect(TOKEN_DO);
    ptr->data.for_data.body = parse_oneline_or_multiline();

    return ptr;
}

/// @brief
Node *parse_value_or_expr(void) {
    Node *ptr = NULL;

    switch (current.type) {
    case TOKEN_IF:
        ptr = parse_if_value();
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

/// @brief
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

/// @brief
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

/// @brief
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

/// @brief
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

/// @brief
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

/// @brief
Node *parse_guard(void) {
    expect(TOKEN_IF);
    return parse_expression();
}

/// @brief
void parse_generator(Node *ptr) {
    ptr->data.for_data.name = expect_id();
    expect(TOKEN_GEN_ARROW);
    ptr->data.for_data.range = parse_expression();
}

/// @brief
Node *parse_st_else_part(void) {
    Node *ptr = NULL;

    if (current.type == TOKEN_EOL) {
        expect(TOKEN_EOL);
        ptr = parse_block();
    } else if (current.type == TOKEN_IF) {
        ptr = parse_if_statement();
    } else if (current.type == TOKEN_RETURN) {
        ptr = parse_return();
    } else if (is_expression_start(current.type)) {
        ptr = parse_expression();
        parse_statement_end();
    } else {
        syntax_error("TOKEN_EOL or expression");
    }

    return ptr;
}

/// @brief
Node *parse_if_value(void) {
    Node *ptr = ASTNewNode(NODE_IF_VALUE, current.line);
    expect(TOKEN_IF);
    ptr->data.if_data.condition = parse_expression();
    expect(TOKEN_THEN);
    parse_value_then_part(ptr);
    return ptr;
}

/// @brief
void parse_value_then_part(Node *ptr) {
    if (current.type == TOKEN_EOL) {
        expect(TOKEN_EOL);
        ptr->data.if_data.then_part = parse_block();
        expect(TOKEN_ELSE);
        ptr->data.if_data.else_part = parse_value_else_part();
    } else if (is_expression_start(current.type)) {
        ptr->data.if_data.then_part = parse_expression();
        ptr->data.if_data.else_part = parse_value_after_inline_part();
    } else {
        syntax_error("TOKEN_EOL or expression");
    }
}


/// @brief
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

/// @brief
Node *parse_value_else_part(void) {
    Node *ptr = NULL;

    if (current.type == TOKEN_EOL) {
        expect(TOKEN_EOL);
        ptr = parse_block();
    } else if (current.type == TOKEN_IF) {
        ptr = parse_if_value();
    } else if (is_expression_start(current.type)) {
        ptr = parse_expression();
        parse_statement_end();
    } else {
        syntax_error("TOKEN_EOL or expression");
    }

    return ptr;
}

/// @brief
Node *parse_function_call(void) {
    Node *ptr = ASTNewNode(NODE_FUNCTION_CALL, current.line);
    ptr->data.call_data.name = expect_id();
    expect(TOKEN_LPAREN);
    parse_list_arguments(&ptr->data.call_data.arguments);
    expect(TOKEN_RPAREN);

    return ptr;
}

/// @brief
void parse_list_arguments(NodeList *arguments) {
    if (current.type == TOKEN_RPAREN) {
        return;

    } else if (is_expression_start(current.type)) {
        NodeListAppend(arguments, parse_argument());
        parse_more_list_arguments(arguments);
    } else {
        syntax_error("exor expression");
    }
}

/// @brief
void parse_more_list_arguments_afrer(NodeList *arguments, unsigned int comma_line) {
    if (current.type == TOKEN_LPAREN) {
        if (comma_line == current.line) {
            syntax_error("LPAREN must start on new line");
        }
    } else if (is_expression_start(current.type)) {
        NodeListAppend(arguments, parse_argument());
        parse_more_list_arguments(arguments);

    } else {
        syntax_error("TOKEN_RPAREN or expression");
    }
}

/// @brief
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

/// @brief
Node *parse_argument(void) {
    return parse_expression();
}

/// @brief
Node *parse_list_literal(void) {
    Node *ptr = ASTNewNode(NODE_LIST_LITERAL, current.line);
    expect(TOKEN_LIST);
    expect(TOKEN_LPAREN);
    parse_list_arguments(&ptr->data.list_data.items);
    expect(TOKEN_RPAREN);

    return ptr;
}

/// @brief
Node *parse(void) {
    initScannerData(&scanner);
    fill_tokens();
    return parse_start();
}

/// @brief
int main(void) {
    Node *root = parse();

#ifdef DEBUG
    ASTprint(root, 0);
#endif

    semantic_analyze(root);
    ASTFree(root);
    TypeFreeAll();
    return 0;
}
