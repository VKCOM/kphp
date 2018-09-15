#include "compiler/vertex.h"

#include "compiler/data.h"

VertexPtr clone_vertex(VertexPtr from) {
  switch (from->type()) {
#define FOREACH_OP(x) case x: {CLONE_VERTEX (res, x, VertexAdaptor <x> (from)); return res;} break;

#include "foreach_op.h"

    default:
    kphp_fail();
  }
  return VertexPtr();
}
