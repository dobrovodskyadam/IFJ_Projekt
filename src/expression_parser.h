// @file expression_parser.h
// @brief
// @date 07-11-2026
//
// @author Lukas Sofka (xsofkal00)
// @author Email: xsofkal00@stud.fit.vutbr.cz
// Faculty of Information Technology, Brno University of Technology

#ifndef EXPRESSION_PARSER_H
#define EXPRESSION_PARSER_H

#include "ast.h"
#include "scanner.h"

extern Token previous, current, next;

#define STACK_INITIAL_CAPACITY 64

Node *parse_expression(void);

#endif // EXPRESSION_PARSER_H
