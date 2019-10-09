#include "compiler/pipes/check-nested-foreach.h"

#include "common/termformat/termformat.h"

#include "compiler/data/var-data.h"

VertexPtr CheckNestedForeachPass::on_enter_vertex(VertexPtr vertex, LocalT *) {
  auto already_used = [&](VarPtr v) {
    return vk::contains(foreach_vars, v) || vk::contains(foreach_key_vars, v);
  };
  if (auto foreach_v = vertex.try_as<op_foreach>()) {
    auto params = foreach_v->params();
    auto x = params->x();
    if (already_used(x->var_id)) {
      kphp_warning (fmt_format("Foreach key {} shadows key or value of outer foreach",
                               TermStringFormat::add_text_attribute("&$" + x->var_id->name, TermStringFormat::bold)));
    }
    foreach_vars.push_back(x->var_id);
    if (x->ref_flag) {
      foreach_ref_vars.push_back(x->var_id);
    }
    if (params->has_key()) {
      auto key = params->key();
      if (already_used(key->var_id)) {
        kphp_warning (fmt_format("Foreach key {} shadows key or value of outer foreach",
                                 TermStringFormat::add_text_attribute("&$" + key->var_id->name, TermStringFormat::bold)));
      }
      foreach_key_vars.push_back(key->var_id);
    }
  }
  if (auto unset = vertex.try_as<op_unset>()) {
    if (auto var = unset->expr().try_as<op_var>()) {
      if (vk::contains(foreach_ref_vars, var->var_id)) {
        kphp_error(0, "Unsetting reference variable is not implemented");
      }
      if (var->var_id->is_foreach_reference) {
        return VertexAdaptor<op_empty>::create();
      }
    }
  }
  if (auto var_v = vertex.try_as<op_var>()) {
    VarPtr var = var_v->var_id;
    if (var->is_foreach_reference) {
      if (!vk::contains(foreach_ref_vars, var) && !vk::contains(errored_vars, var)) {
        errored_vars.push_back(var);
        kphp_error(0, fmt_format("Foreach reference variable {} used outside of loop",
                                 TermStringFormat::add_text_attribute("&$" + var->name, TermStringFormat::bold)));
      }
    }
  }
  return vertex;
}
VertexPtr CheckNestedForeachPass::on_exit_vertex(VertexPtr vertex, LocalT *) {
  if (auto foreach_v = vertex.try_as<op_foreach>()) {
    auto params = foreach_v->params();
    foreach_vars.pop_back();
    if (params->x()->ref_flag) {
      foreach_ref_vars.pop_back();
    }
    if (params->has_key()) {
      foreach_key_vars.pop_back();
    }
  }
  return vertex;
}
