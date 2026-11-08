// @file error.h
// @brief
// @date 06-11-2026
//
// @author Lukas Sofka (xsofkal00)
// @author Email: xsofkal00@stud.fit.vutbr.cz
// Faculty of Information Technology, Brno University of Technology

// TODO: add the rest of the error codes
typedef enum {
    NO_ERR = 0,
    ERR_LEXICAL = 1,
    ERR_SYNTAX = 2,
} ErrorCode;

void error_exit(ErrorCode code, unsigned line, const char *fmt, ...);
