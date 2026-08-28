// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/vertex.h"

VertexPtr clone_vertex(VertexPtr from) {
  switch (from->type()) {
#define FOREACH_OP(x)                                                                                                                                          \
  case x: {                                                                                                                                                    \
    return VertexPtr(raw_clone_vertex_inner<x>(*from.as<x>()));                                                                                                \
  } break;

#include "auto/compiler/vertex/foreach-op.h"

  default:
    kphp_fail();
  }
  return {};
}
