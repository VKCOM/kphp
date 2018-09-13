#include "compiler/vertex.h"

#include "compiler/data.h"

VertexPtr create_vertex(Operation op, VertexPtr first) {
  switch (op) {
#define FOREACH_OP(x) case x: {CREATE_VERTEX (res, x, first); return res;} break;

#include "foreach_op.h"

    default:
    kphp_fail();
  }
  return VertexPtr();
}

VertexPtr create_vertex(Operation op, VertexPtr first, VertexPtr second) {
  switch (op) {
#define FOREACH_OP(x) case x: {CREATE_VERTEX (res, x, first, second); return res;} break;

#include "foreach_op.h"

    default:
    kphp_fail();
  }
  return VertexPtr();
}

VertexPtr clone_vertex(VertexPtr from) {
  switch (from->type()) {
#define FOREACH_OP(x) case x: {CLONE_VERTEX (res, x, VertexAdaptor <x> (from)); return res;} break;

#include "foreach_op.h"

    default:
    kphp_fail();
  }
  return VertexPtr();
}
