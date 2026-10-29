// @file parser.c
// @brief
// @date 23-10-2026
//
// @author Lukas Sofka (xsofkal00)
// @author Email: xsofkal00@stud.fit.vutbr.cz
// Faculty of Information Technology, Brno University of Technology

#include "parser.h"
#include "scanner.h"
#include "symtable.h"

#include <stdbool.h>
#include <stdio.h>


int main(void) {

    ScannerData sc;
    Token tok;


    // TODO
    // First(X)
    // Follow(X)
    // Implementace LL(1) gramatiky

    initScannerData(&sc);
    do {
        getToken(&tok, &sc);
        printf("%s\n", tokenTypeName(tok.type));
    } while (tok.type != TOKEN_EOF);
}
