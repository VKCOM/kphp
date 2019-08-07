#include "compiler/pipes/transform-to-smart-instanceof.h"

#include <memory>

#include "common/containers/final_action.h"

#include "compiler/class-assumptions.h"
#include "compiler/const-manipulations.h"
#include "compiler/gentree.h"
#include "compiler/name-gen.h"

bool TransformToSmartInstanceof::user_recursion(VertexPtr v, LocalT *, VisitVertex<TransformToSmartInstanceof> &visit) {
  auto if_vertex = v.try_as<op_if>();
  auto condition = get_instanceof_from_if(if_vertex);
  if (!condition) {
    return false;
  }

  auto name_of_derived_vertex = condition->rhs();
  visit(if_vertex->cond());
  auto initial_state = variable_state;

  visit_cmd(visit, if_vertex, name_of_derived_vertex, if_vertex->true_cmd_ref());
  variable_state = initial_state;

  if (!if_vertex->has_false_cmd()) {
    return true;
  }

  auto _ = vk::finally([this, &initial_state] () mutable { variable_state = std::move(initial_state); });
  auto instance_var = condition->lhs();
  auto &state = variable_state[instance_var->get_string()];
  if (!fill_derived_classes(instance_var, name_of_derived_vertex, state)) {
    return true;
  }

  if (state.left_derived.size() == 1) {
    auto false_cmd = if_vertex->false_cmd();
    auto condition_inside_false = false_cmd->size() == 1 ? get_instanceof_from_if(false_cmd->args()[0].try_as<op_if>()) : VertexAdaptor<op_instanceof>{};
    auto is_instanceof_the_same_variable = condition_inside_false && condition_inside_false->lhs()->get_string() == instance_var->get_string();
    bool left_derived_is_interface = !(*state.left_derived.begin())->parent_class || (*state.left_derived.begin())->parent_class->is_interface();
    if (left_derived_is_interface && !is_instanceof_the_same_variable) {
      auto derived_name_vertex = VertexAdaptor<op_string>::create();
      derived_name_vertex->set_string((*state.left_derived.begin())->name);

      visit_cmd(visit, if_vertex, derived_name_vertex, if_vertex->false_cmd_ref());
      return true;
    }
  }

  visit(if_vertex->false_cmd_ref());
  return true;
}

void TransformToSmartInstanceof::visit_cmd(VisitVertex<TransformToSmartInstanceof> &visit, VertexAdaptor<op_if> if_vertex, VertexPtr name_of_derived_vertex, VertexPtr &cmd) {
  auto condition = get_instanceof_from_if(if_vertex);
  auto instance_var = condition->lhs();

  auto &state = variable_state[instance_var->get_string()];

  auto set_instance_cast_to_tmp = generate_tmp_var_with_instance_cast(instance_var.clone(), name_of_derived_vertex, state.new_name);
  cmd = VertexAdaptor<op_seq>::create(set_instance_cast_to_tmp, cmd.as<op_seq>()->args()).set_location(cmd);
  auto commands = cmd.as<op_seq>()->args();

  std::for_each(std::next(commands.begin()), commands.end(), visit);
}

bool TransformToSmartInstanceof::is_good_var_for_instanceof(VertexPtr v) {
  return vk::any_of_equal(v->type(), op_var);
}

VertexPtr TransformToSmartInstanceof::on_enter_vertex(VertexPtr v, FunctionPassBase::LocalT *) {
  if (!is_good_var_for_instanceof(v) || variable_state.empty()) {
    return v;
  }

  while (true) {
    auto replacement_it = variable_state.find(v->get_string());
    if (replacement_it == variable_state.end() || replacement_it->second.new_name.empty()) {
      break;
    }

    v->set_string(replacement_it->second.new_name);
  }

  return v;
}

VertexAdaptor<op_set> TransformToSmartInstanceof::generate_tmp_var_with_instance_cast(VertexPtr instance_var, VertexPtr derived_name_vertex, std::string &new_name) {
  auto cast_to_derived = VertexAdaptor<op_func_call>::create(instance_var, derived_name_vertex);
  cast_to_derived->set_string("instance_cast");

  auto tmp_var = VertexAdaptor<op_var>::create();
  new_name = gen_unique_name(instance_var->get_string());
  tmp_var->set_string(new_name);
  tmp_var->extra_type = op_ex_var_superlocal_inplace;
  tmp_var->is_const = true;

  auto set_instance_cast_to_tmp = VertexAdaptor<op_set>::create(tmp_var, cast_to_derived);
  set_location(instance_var->get_location(), cast_to_derived, tmp_var, set_instance_cast_to_tmp, derived_name_vertex);

  return set_instance_cast_to_tmp;
}

VertexAdaptor<op_instanceof> TransformToSmartInstanceof::get_instanceof_from_if(VertexAdaptor<op_if> if_vertex) {
  if (if_vertex) {
    if (auto condition = if_vertex->cond().try_as<op_conv_bool>()) {
      return condition->expr().try_as<op_instanceof>();
    }
  }

  return VertexAdaptor<op_instanceof>{};
}

bool TransformToSmartInstanceof::fill_derived_classes(VertexPtr instance_var, VertexPtr name_of_derived_vertex, TransformToSmartInstanceof::NewNameAndLeftDerived &state) {
  if (state.left_derived.empty()) {
    InterfacePtr interface_class;
    auto assum = infer_class_of_expr(current_function, instance_var, interface_class);
    kphp_error_act(assum == AssumType::assum_instance,
      format("variable is not an instance: `%s`", instance_var->get_string().c_str()),return false);
    state.left_derived = {interface_class->derived_classes.begin(), interface_class->derived_classes.end()};
  }

  const auto &derived_class_name = GenTree::get_actual_value(name_of_derived_vertex)->get_string();
  auto derived_class = G->get_class(derived_class_name);
  state.left_derived.erase(derived_class);

  return true;
}

