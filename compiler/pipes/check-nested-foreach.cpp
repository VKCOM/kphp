// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/check-nested-foreach.h"

#include "common/algorithms/contains.h"
#include "common/termformat/termformat.h"

#include "compiler/data/var-data.h"

VarPtr get_non_local_scope_visible_var(VertexAdaptor<op_foreach_param> params) noexcept {
  if (auto iterable_instance = params->xs().try_as<op_instance_prop>()) {
    return iterable_instance->var_id;
  }

  if (auto iterable_var = params->xs().try_as<op_var>()) {
    if (iterable_var->var_id->is_in_global_scope()) {
      return iterable_var->var_id;
    }
  }

  return {};
}

VertexPtr CheckNestedForeachPass::on_enter_vertex(VertexPtr vertex) {
  auto already_used = [&](VarPtr v) {
    return vk::contains(foreach_vars_, v) || vk::contains(foreach_key_vars_, v);
  };
  if (auto foreach_v = vertex.try_as<op_foreach>()) {
    auto params = foreach_v->params();
    auto x = params->x();
    if (already_used(x->var_id)) {
      kphp_warning (fmt_format("Foreach key {} shadows key or value of outer foreach",
                               TermStringFormat::add_text_attribute("&$" + x->var_id->name, TermStringFormat::bold)));
    }
    foreach_vars_.push_back(x->var_id);
    if (x->ref_flag) {
      foreach_ref_vars_.push_back(x->var_id);
    }
    if (params->has_key()) {
      auto key = params->key();
      if (already_used(key->var_id)) {
        kphp_warning (fmt_format("Foreach key {} shadows key or value of outer foreach",
                                 TermStringFormat::add_text_attribute("&$" + key->var_id->name, TermStringFormat::bold)));
      }
      foreach_key_vars_.push_back(key->var_id);
    }
    if (auto iterable_var = get_non_local_scope_visible_var(params)) {
      foreach_iterable_non_local_vars_.emplace_back(iterable_var);
    }
  }
  if (auto unset = vertex.try_as<op_unset>()) {
    if (auto var = unset->expr().try_as<op_var>()) {
      if (vk::contains(foreach_ref_vars_, var->var_id)) {
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
      if (!vk::contains(foreach_ref_vars_, var) && !vk::contains(errored_vars_, var)) {
        errored_vars_.push_back(var);
        kphp_error(0, fmt_format("Foreach reference variable {} used outside of loop",
                                 TermStringFormat::add_text_attribute("&$" + var->name, TermStringFormat::bold)));
      }
    }
  }
  if (!foreach_iterable_non_local_vars_.empty()) {
    if (auto call = vertex.try_as<op_func_call>()) {
      if (call->func_id->is_resumable || call->func_id->can_be_implicitly_interrupted_by_other_resumable || call->func_id->is_light_fork) {
        kphp_error(foreach_ref_vars_.empty(),
                   fmt_format("It is forbidden to call resumable functions inside foreach with reference by var {} with non local scope visibility",
                              foreach_iterable_non_local_vars_.back()->as_human_readable()));
      }
    }
  }
  return vertex;
}

VertexPtr CheckNestedForeachPass::on_exit_vertex(VertexPtr vertex) {
  if (auto foreach_v = vertex.try_as<op_foreach>()) {
    auto params = foreach_v->params();
    foreach_vars_.pop_back();
    if (params->x()->ref_flag) {
      foreach_ref_vars_.pop_back();
    }
    if (params->has_key()) {
      foreach_key_vars_.pop_back();
    }
    if (auto iterable_var = get_non_local_scope_visible_var(params)) {
      kphp_assert(foreach_iterable_non_local_vars_.back() == iterable_var);
      foreach_iterable_non_local_vars_.pop_back();
    }
  }
  return vertex;
}
