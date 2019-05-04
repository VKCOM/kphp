#ifndef PHP_DEBUG_H
#define PHP_DEBUG_H

#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/operation.h"
#include "compiler/token.h"

std::string debugOperationName(Operation o);
std::string debugTokenName(TokenType t);
void debugPrintVertexTree(VertexPtr root, int level = 0);
void debugPrintFunction(FunctionPtr function);
std::string debugVertexMore(VertexPtr v);

#endif //PHP_DEBUG_H
