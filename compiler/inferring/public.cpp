#include "compiler/inferring/public.h"

#include "compiler/data/function-data.h"
#include "compiler/data/var-data.h"
#include "compiler/vertex.h"

static void init_functions_tinf_nodes(FunctionPtr function) {
  assert (function->tinf_state == 1);
  VertexRange params = function->get_params();
  function->tinf_nodes.resize(params.size() + 1);
  if (function->type() != FunctionData::func_extern) {
    kphp_assert_msg(function->param_ids.size() == params.size(),
      "This could happen due to wrong FunctionPtr set into Location inside vertex");
  }
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

TypeInferer *get_inferer() {
  static TypeInferer inferer;
  return &inferer;
}

inline const TypeData *get_type_impl(Node *node) {
  if (node->get_recalc_cnt() == -1) {
    get_inferer()->run_node(node);
  }
  return node->type_;
}

const TypeData *get_type(VertexPtr vertex) {
  return get_type_impl(get_tinf_node(vertex));
}

const TypeData *get_type(VarPtr var) {
  return get_type_impl(get_tinf_node(var));
}

const TypeData *get_type(FunctionPtr function, int id) {
  return get_type_impl(get_tinf_node(function, id));
}

}
