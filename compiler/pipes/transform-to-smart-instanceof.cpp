// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/transform-to-smart-instanceof.h"

#include "common/algorithms/contains.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/name-gen.h"

namespace {
  ClassPtr get_first_common_child(ClassPtr base1, ClassPtr base2) {
    auto base1_children = base1->get_all_inheritors();
    std::unordered_set<ClassPtr> derived_base1{base1_children.begin(), base1_children.end()};

    for (auto base2_child : base2->get_all_inheritors()) {
      if (vk::contains(derived_base1, base2_child)) {
        return base2_child;
      }
    }

    return {};
  }
} // namespace

bool TransformToSmartInstanceof::user_recursion(VertexPtr v) {
  auto if_vertex = v.try_as<op_if>();
  auto condition = get_instanceof_from_if(if_vertex);
  auto instance_var = condition ? condition->lhs().try_as<op_var>().clone() : VertexAdaptor<op_var>{};
  if (!instance_var) {
    return false;
  }

  auto type_of_lhs = infer_class_of_expr(current_function, instance_var).try_as_class();
  kphp_error_act(type_of_lhs, "left operand of `instanceof` should be instance of class", return false);

  run_function_pass(if_vertex->cond(), this);

  auto name_of_rhs_class_vertex = condition->rhs().as<op_string>();
  const std::string &name_of_rhs_class = name_of_rhs_class_vertex->str_val;
  ClassPtr rhs_class = G->get_class(name_of_rhs_class);
  kphp_error_act(rhs_class, fmt_format("can't find right operand of `instanceof`: {}", name_of_rhs_class), return false);

  if (!rhs_class->is_parent_of(type_of_lhs) && !type_of_lhs->is_parent_of(rhs_class)) {
    rhs_class = get_first_common_child(rhs_class, type_of_lhs);
    kphp_error_act(rhs_class, fmt_format("{} and {} are not compatible, always false", type_of_lhs->name, name_of_rhs_class), return false);
    name_of_rhs_class_vertex = name_of_rhs_class_vertex.clone();
    name_of_rhs_class_vertex->str_val = rhs_class->name;
  }

  add_tmp_var_with_instance_cast(instance_var, name_of_rhs_class_vertex, if_vertex->true_cmd_ref());
  new_names_of_var[instance_var->str_val].pop();

  if (if_vertex->has_false_cmd()) {
    run_function_pass(if_vertex->false_cmd_ref(), this);
  }
  return true;
}

void TransformToSmartInstanceof::add_tmp_var_with_instance_cast(VertexAdaptor<op_var> instance_var, VertexPtr name_of_derived, VertexPtr &cmd) {
  auto set_instance_cast_to_tmp = generate_tmp_var_with_instance_cast(instance_var, name_of_derived);
  auto &name_of_tmp_var = set_instance_cast_to_tmp->lhs().as<op_var>()->str_val;
  new_names_of_var[instance_var->str_val].push(name_of_tmp_var);

  cmd = VertexAdaptor<op_seq>::create(set_instance_cast_to_tmp, cmd.as<op_seq>()->args()).set_location(cmd);
  auto commands = cmd.as<op_seq>()->args();

  std::for_each(std::next(commands.begin()), commands.end(), [&](VertexPtr &v) {
    return run_function_pass(v, this);
  });
}

VertexPtr TransformToSmartInstanceof::on_enter_vertex(VertexPtr v) {
  if (v->type() != op_var || new_names_of_var.empty()) {
    return v;
  }

  auto replacement_it = new_names_of_var.find(v->get_string());
  if (replacement_it != new_names_of_var.end() && !replacement_it->second.empty()) {
    v->set_string(replacement_it->second.top());
  }

  return v;
}

VertexAdaptor<op_set> TransformToSmartInstanceof::generate_tmp_var_with_instance_cast(VertexPtr instance_var, VertexPtr derived_name_vertex) {
  auto cast_to_derived = VertexAdaptor<op_func_call>::create(instance_var, derived_name_vertex).set_location(instance_var);
  cast_to_derived->set_string("instance_cast");

  auto tmp_var = VertexAdaptor<op_var>::create().set_location(instance_var);
  tmp_var->set_string(gen_unique_name(instance_var->get_string()));
  tmp_var->extra_type = op_ex_var_superlocal;
  tmp_var->is_const = true;

  auto set_instance_cast_to_tmp = VertexAdaptor<op_set>::create(tmp_var, cast_to_derived).set_location(instance_var);
  derived_name_vertex.set_location(instance_var);

  return set_instance_cast_to_tmp;
}

VertexAdaptor<op_instanceof> TransformToSmartInstanceof::get_instanceof_from_if(VertexAdaptor<op_if> if_vertex) {
  if (if_vertex) {
    if (auto condition = if_vertex->cond().try_as<op_conv_bool>()) {
      return condition->expr().try_as<op_instanceof>();
    }
  }

  return {};
}

