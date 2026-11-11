// @file error.h
// @brief Header file with Error Codes
// @date 06-11-2026
//
// @author Lukas Sofka (xsofkal00)
// @author Email: xsofkal00@stud.fit.vutbr.cz
// Faculty of Information Technology, Brno University of Technology

// clang-format off
typedef enum {
    NO_ERR                       = 0,
    ERR_LEXICAL                  = 1,  // wrong lexeme or indentation
    ERR_SYNTAX                   = 2,  // wrong syntax, missing prolog, broken block structure
    ERR_SEMANTIC_UNDEFINED       = 3,  // undefined function, variable or type, missing @main
    ERR_SEMANTIC_REDEFINITION    = 4,  // redefinition of function, variable or type
    ERR_SEMANTIC_ARGUMENTS       = 5,  // wrong @main function or incorrect number of arguments
    ERR_SEMANTIC_TYPE            = 6,  // type incompatibility, return incompability, other type issues
    ERR_SEMANTIC_IMMUTABLE       = 7,  // attempt to write to val or function parameter
    ERR_SEMANTIC_OTHER           = 10, // other semantic errors (e.g. wrong return, zero range step)
    ERR_INTERNAL                 = 99  // internall errro of compiler (e.g. allocation error)
} ErrorCode;

void error_exit(ErrorCode code, unsigned line, const char *fmt, ...);
