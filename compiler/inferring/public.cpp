#include "compiler/inferring/public.h"

#include "compiler/data/function-data.h"
#include "compiler/data/var-data.h"
#include "compiler/vertex.h"

static void init_functions_tinf_nodes(FunctionPtr function) {
  assert (function->tinf_state == 1);
  VertexRange params = function->get_params();
  function->tinf_nodes.resize(params.size() + 1);
  for (int i = 0; i < (int)function->tinf_nodes.size(); i++) {
    function->tinf_nodes[i].param_i = i - 1;
    function->tinf_nodes[i].function_ = function;
    if (i && function->type() != FunctionData::func_extern) {
      function->tinf_nodes[i].var_ = function->param_ids[i - 1];
    }
  }
}

namespace tinf {

Node *get_tinf_node(FunctionPtr function, int id) {
  if (function->tinf_state == 0) {
    if (__sync_bool_compare_and_swap(&function->tinf_state, 0, 1)) {
      init_functions_tinf_nodes(function);
      __sync_synchronize();
      function->tinf_state = 2;
    }
  }
  while (function->tinf_state != 2) {
    __sync_synchronize();
  }

  assert (-1 <= id && id + 1 < (int)function->tinf_nodes.size());
  return &function->tinf_nodes[id + 1];
}

Node *get_tinf_node(VertexPtr vertex) {
  return &vertex->tinf_node;
}

Node *get_tinf_node(VarPtr vertex) {
  return &vertex->tinf_node;
}

const TypeData *fast_get_type(VertexPtr vertex) {
  return get_tinf_node(vertex)->get_type();
}

const TypeData *fast_get_type(VarPtr var) {
  return get_tinf_node(var)->get_type();
}

const TypeData *fast_get_type(FunctionPtr function, int id) {
  return get_tinf_node(function, id)->get_type();
}

static TypeInferer *CI = nullptr;

void register_inferer(TypeInferer *inferer) {
  if (!__sync_bool_compare_and_swap(&CI, nullptr, inferer)) {
    kphp_fail();
  }
}

TypeInferer *get_inferer() {
  return CI;
}

const TypeData *get_type(VertexPtr vertex) {
  return get_inferer()->get_type(get_tinf_node(vertex));
}

const TypeData *get_type(VarPtr var) {
  return get_inferer()->get_type(get_tinf_node(var));
}

const TypeData *get_type(FunctionPtr function, int id) {
  return get_inferer()->get_type(get_tinf_node(function, id));
}

}
