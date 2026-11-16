/// @file semantic.h
/// @brief
/// @date 12-11-2026
///
/// @author Lukas Sofka (xsofkal00)
/// @author Email: xsofkal00@stud.fit.vutbr.cz
/// Faculty of Information Technology, Brno University of Technology

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "error.h"
#include "symtable.h"


// Main Semantic Analyzer
int semantic_analyze(Node *root);

// Helper Semantic Functions
void collect_functions(symtab_t *symtable, Node *root);

#endif // SEMANTIC_H
