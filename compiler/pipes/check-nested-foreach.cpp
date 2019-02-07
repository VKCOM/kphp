#include "compiler/pipes/check-nested-foreach.h"

#include "compiler/data/var-data.h"

void CheckNestedForeachPass::init() {
  in_unset = 0;
  forbidden_vars = vector<VarPtr>();
  foreach_vars = vector<VarPtr>();
}
VertexPtr CheckNestedForeachPass::on_enter_vertex(VertexPtr vertex, CheckNestedForeachPass::LocalT *local) {
  local->to_remove = 0;
  local->to_forbid = VarPtr();
  if (auto foreach_v = vertex.try_as<op_foreach>()) {
    auto params = foreach_v->params().as<op_foreach_param>();
    VertexPtr xs = params->xs();
    while (xs->type() == op_index) {
      xs = xs.as<op_index>()->array();
    }
    if (xs->type() == op_var) {
      VarPtr xs_var = xs.as<op_var>()->get_var_id();
      foreach_vars.push_back(xs_var);
      local->to_remove++;
    }
    VertexPtr x = params->x();
    kphp_assert (x->type() == op_var);
    VarPtr x_var = x.as<op_var>()->get_var_id();
    for (int i = 0; i < foreach_vars.size(); i++) {
      if (x_var->name == foreach_vars[i]->name) {
        kphp_warning (format("Foreach value \"%s\" shadows array, key or value of outer foreach", x_var->name.c_str()));
      }
    }
    foreach_vars.push_back(x_var);
    local->to_remove++;
    if (x->ref_flag) {
      local->to_forbid = x_var;
      for (int i = 0; i < forbidden_vars.size(); i++) {
        if (forbidden_vars[i]->name == x_var->name) {
          std::swap(forbidden_vars[i], forbidden_vars.back());
          forbidden_vars.pop_back();
          break;
        }
      }
    }
    if (params->has_key()) {
      local->to_remove++;
      VertexPtr key = params->key();
      kphp_assert (key->type() == op_var);
      VarPtr key_var = key.as<op_var>()->get_var_id();
      for (int i = 0; i < foreach_vars.size(); i++) {
        if (key_var->name == foreach_vars[i]->name) {
          kphp_warning (format("Foreach key \"%s\" shadows array, key or value of outer foreach", key_var->name.c_str()));
        }
      }
      foreach_vars.push_back(key_var);
    }
  }
  if (vertex->type() == op_unset) {
    in_unset++;
  }
  if (vertex->type() == op_var && !in_unset) {
    VarPtr var = vertex.as<op_var>()->get_var_id();
    for (int i = 0; i < forbidden_vars.size(); i++) {
      if (var->name == forbidden_vars[i]->name) {
        kphp_warning (format("Reference foreach value \"%s\" is used after foreach", var->name.c_str()));
        std::swap(forbidden_vars[i], forbidden_vars.back());
        forbidden_vars.pop_back();
        break;
      }
    }
  }
  return vertex;
}
VertexPtr CheckNestedForeachPass::on_exit_vertex(VertexPtr vertex, CheckNestedForeachPass::LocalT *local) {
  for (int i = 0; i < local->to_remove; i++) {
    kphp_assert(foreach_vars.size());
    foreach_vars.pop_back();
  }
  if (local->to_forbid) {
    forbidden_vars.push_back(local->to_forbid);
  }
  if (vertex->type() == op_unset) {
    in_unset--;
  }
  return vertex;
}
