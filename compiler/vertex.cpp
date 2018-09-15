#include "compiler/vertex.h"

#include "compiler/data.h"

VertexPtr clone_vertex(VertexPtr from) {
  switch (from->type()) {
#define FOREACH_OP(x) case x: {return VertexPtr (raw_clone_vertex_inner <x> (*from.as<x>()));} break;

#include "foreach_op.h"

    default:
    kphp_fail();
  }
  return VertexPtr();
}
