// @file error.c
// @brief
// @date 06-11-2026
//
// @author Lukas Sofka (xsofkal00)
// @author Email: xsofkal00@stud.fit.vutbr.cz
// Faculty of Information Technology, Brno University of Technology

#include "error.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void error_exit(ErrorCode code, const char *fmt, ...) {
#ifdef DEBUG_MSG
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
#else
    // For supressing warning: unused parameter ‘fmt’ [-Wunused-parameter]
    (void)fmt;
#endif
    exit(code);
}
