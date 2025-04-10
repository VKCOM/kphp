// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/analyzer.h"

#include "common/algorithms/string-algorithms.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/define-data.h"
#include "compiler/data/var-data.h"
#include "compiler/inferring/public.h"
#include "compiler/utils/string-utils.h"

namespace {

std::set<std::string> collect_duplicate_keys(VertexPtr to_check) {
  bool have_arrow = false;
  bool have_int_key = false;
  std::set<std::string> used_keys;
  std::set<std::string> duplicates;
  int id = 0;
  for (auto v : *to_check) {
    if (v->type() == op_double_arrow) {
      have_arrow = true;
      VertexPtr key = v.as<op_double_arrow>()->key();
      have_int_key |= key->type() == op_int_const;
      std::string str;
      if (key->type() == op_string || key->type() == op_int_const) {
        str = key->get_string();
      } else if (key->type() == op_var) {
        VarPtr key_var = key.as<op_var>()->var_id;
        if (key_var->is_constant()) {
          VertexPtr init = key_var->init_val;
          if (init->type() == op_string) {
            str = init->get_string();
          }
        }
      }
      if (!str.empty() && !used_keys.emplace(str).second) {
        duplicates.emplace(std::move(str));
      }
    } else {
      if (have_arrow && have_int_key) {
        break;
      }
      used_keys.emplace(std::to_string(id++));
    }
  }
  return duplicates;
}

void check_var_init_value(const VarPtr& var) {
  if (auto var_init_const = var->init_val.try_as<op_var>()) {
    if (auto const_array = var_init_const->var_id->init_val.try_as<op_array>()) {
      auto duplications = collect_duplicate_keys(const_array);
      if (!duplications.empty()) {
        const char* static_prefix = (var->is_function_static_var() || var->is_class_static_var()) ? "static " : "";
        stage::set_location(const_array->get_location());
        kphp_warning(
            fmt_format("Got array duplicate keys ['{}'] in '{}{}' init value", vk::join(duplications, "', '"), static_prefix, var->as_human_readable()));
      }
    }
  }
}

} // namespace

void CommonAnalyzerPass::check_set(VertexAdaptor<op_set> to_check) {
  VertexPtr left = to_check->lhs();
  VertexPtr right = to_check->rhs();
  if (left->type() == op_var && right->type() == op_var) {
    VarPtr lvar = left.as<op_var>()->var_id;
    VarPtr rvar = right.as<op_var>()->var_id;
    if (lvar->name == rvar->name) {
      kphp_warning("Assigning variable to itself\n");
    }
  }
}

VertexPtr CommonAnalyzerPass::on_enter_vertex(VertexPtr vertex) {
  if (vertex->type() == op_array) {
    auto duplications = collect_duplicate_keys(vertex);
    if (!duplications.empty()) {
      kphp_warning(fmt_format("Got array duplicate keys ['{}']", vk::join(duplications, "', '")));
    }
    return vertex;
  }
  if (vertex->type() == op_var) {
    VarPtr var = vertex.as<op_var>()->var_id;
    if (var->is_constant()) {
      run_function_pass(var->init_val, this);
    }
    if (var->is_in_global_scope()) {
      // save a flag, that a function's body accesses mutable global / static vars (to codegen `php_globals` variable)
      // note, that assigning `has_global_vars_inside = !global_var_ids.empty()` is incorrect:
      // for example if a function declares `global $v` but not uses it, or its uses are dropped off after cfg pass
      current_function->has_global_vars_inside = true;
    }
    return vertex;
  }
  if (vertex->rl_type == val_none) {
    if (OpInfo::rl(vertex->type()) == rl_op && OpInfo::cnst(vertex->type()) == cnst_const_func) {
      if (vk::none_of_equal(vertex->type(), op_log_and, op_log_or, op_ternary, op_log_and_let, op_log_or_let, op_unset)) {
        kphp_warning(fmt_format("Statement has no effect [op = {}]", OpInfo::P[vertex->type()].str));
      }
    }
  }
  if (vertex->type() == op_set) {
    // TODO: $x = (int)$x;
    // check_set(vertex.as<op_set>());
    return vertex;
  }
  return vertex;
}

void CommonAnalyzerPass::on_finish() {
  for (VarPtr& var : current_function->local_var_ids) {
    G->stats.cnt_mixed_vars += tinf::get_type(var)->ptype() == tp_mixed;
  }

  for (VarPtr& var : current_function->param_ids) {
    G->stats.cnt_mixed_params += tinf::get_type(var)->ptype() == tp_mixed;
    G->stats.cnt_const_mixed_params += (tinf::get_type(var)->ptype() == tp_mixed) && var->is_read_only;
  }

  if (current_function->type == FunctionData::func_class_holder) {
    current_function->class_id->members.for_each([](ClassMemberStaticField& field) { check_var_init_value(field.var); });
    current_function->class_id->members.for_each([](ClassMemberInstanceField& field) { check_var_init_value(field.var); });
  }

  std::for_each(current_function->static_var_ids.begin(), current_function->static_var_ids.end(), check_var_init_value);
  return FunctionPassBase::on_finish();
}
