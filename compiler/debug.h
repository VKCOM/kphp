#ifndef PHP_DEBUG_H
#define PHP_DEBUG_H

#include "compiler/token.h"
#include "compiler/operation.h"
#include "compiler/data.h"


std::string debugOperationName(Operation o);
std::string debugTokenName(TokenType t);
void debugPrintVertexTree(VertexPtr root, int level = 0);
void debugPrintFunction(FunctionPtr function);
std::string debugVertexMore(VertexPtr v);

#endif //PHP_DEBUG_H
