// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/code-gen/code-generator.h"
#include "compiler/data/vertex-adaptor.h"

void compile_vertex(VertexPtr root, CodeGenerator &W);

template<Operation Op>
CodeGenerator& operator<<(CodeGenerator &W, VertexAdaptor<Op> root) {
  compile_vertex(root, W);
  return W;
}
