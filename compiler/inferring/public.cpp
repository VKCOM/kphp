// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/inferring/public.h"

#include "compiler/data/function-data.h"
#include "compiler/data/var-data.h"
#include "compiler/vertex.h"

namespace tinf {

Node* get_tinf_node(FunctionPtr function, int param_i) {
  if (param_i == -1) {
    return &function->tinf_node;
  }

  kphp_assert(param_i >= 0 && param_i < function->param_ids.size());
  return &function->param_ids[param_i]->tinf_node;
}

Node* get_tinf_node(VertexPtr vertex) {
  return &vertex->tinf_node;
}

Node* get_tinf_node(VarPtr vertex) {
  return &vertex->tinf_node;
}

TypeInferer* get_inferer() {
  static TypeInferer inferer;
  return &inferer;
}

inline const TypeData* get_type_impl(Node* node) {
  if (!node->was_recalc_started_at_least_once()) {
    get_inferer()->run_node(node);
  }
  return node->get_type();
}

const TypeData* get_type(VertexPtr vertex) {
  return get_type_impl(get_tinf_node(vertex));
}

const TypeData* get_type(VarPtr var) {
  return get_type_impl(get_tinf_node(var));
}

const TypeData* get_type(FunctionPtr function, int param_i) {
  return get_type_impl(get_tinf_node(function, param_i));
}

} // namespace tinf
