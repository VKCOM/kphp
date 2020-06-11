#include "compiler/data/lambda-class-data.h"

#include <string>

#include "compiler/class-assumptions.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/gentree.h"
#include "compiler/inferring/public.h"
#include "compiler/utils/string-utils.h"
#include "compiler/vertex.h"

LambdaPtr LambdaClassData::get_from(VertexPtr v) {
  if (auto func_ptr = v.try_as<op_func_ptr>()) {
    if (func_ptr->func_id) {
      return get_from(func_ptr->func_id->root);
    }
  }

  if (tinf::Node *tinf_node = tinf::get_tinf_node(v)) {
    if (tinf_node->get_type()->ptype() != tp_Unknown) {
      return tinf_node->get_type()->class_type().try_as<LambdaClassData>();
    }
  }

  if (auto function = v.try_as<op_function>()) {
    return function->func_id->class_id.try_as<LambdaClassData>();
  } else if (vk::any_of_equal(v->type(), op_func_call, op_var)) {
    auto as_instance = infer_class_of_expr(stage::get_function(), v).try_as<AssumInstance>();
    if (as_instance) {
      return as_instance->klass.try_as<LambdaClassData>();
    }
  }

  return {};
}

void LambdaClassData::infer_uses_assumptions(FunctionPtr parent_function) {
  members.for_each([this, parent_function](const ClassMemberInstanceField &field) {
    vk::intrusive_ptr<Assumption> assumption;
    auto local_name = field.local_name();
    if (local_name == LambdaClassData::get_parent_this_name()) {
      if (parent_function->is_lambda()) {
        assumption = assumption_get_for_var(parent_function->class_id, LambdaClassData::get_parent_this_name());
      } else {
        local_name = "this";
      }
    }
    assumptions_for_vars.emplace_back(std::string{field.local_name()},
                                      assumption ?: calc_assumption_for_var(parent_function, local_name));
  });
}

void LambdaClassData::implement_interface(InterfacePtr interface) {
  implements.emplace_back(interface);
  add_virt_clone();
  add_class_constant();
  register_defines();

  auto my_invoke_method = members.get_instance_method(ClassData::NAME_OF_INVOKE_METHOD)->function;
  infer_uses_assumptions(stage::get_function());
  my_invoke_method->is_template = false;
  for (auto &p : my_invoke_method->get_params()) {
    auto param = p.as<meta_op_func_param>();
    param->template_type_id = -1;
    param->is_callable = false;
  }

  // TODO: add kphp_infer for __invoke method of interface
  // interface->has_kphp_infer = true;
}

bool LambdaClassData::can_implement_interface(InterfacePtr interface) const {
  if (interface->members.has_any_static_method() || interface->members.count_of_instance_methods() != 1 || !interface->implements.empty()) {
    return false;
  }

  if (auto invoke_method_from_interface = interface->members.get_instance_method(ClassData::NAME_OF_INVOKE_METHOD)) {
    auto my_invoke_method = members.get_instance_method(ClassData::NAME_OF_INVOKE_METHOD);
    return my_invoke_method && invoke_method_from_interface->function->get_min_argn() == my_invoke_method->function->get_min_argn();
  }

  return false;
}

PrimitiveType infer_type_of_callback_arg(VertexPtr type_rule, VertexAdaptor<op_func_call> extern_function_call,
                                         FunctionPtr function_context, vk::intrusive_ptr<Assumption> &assumption) {
  if (auto lca_rule = type_rule.try_as<op_type_expr_lca>()) {
    PrimitiveType result_pt = tp_Unknown;
    for (auto v : lca_rule->args()) {
      PrimitiveType pt = infer_type_of_callback_arg(v, extern_function_call, function_context, assumption);
      if (assumption && !assumption->is_primitive()) {
        return pt;
      }

      if (result_pt == tp_Unknown) {
        result_pt = pt;
      }
    }

    return result_pt != tp_Unknown ? result_pt : tp_Any;
  } else if (auto callback_call_rule = type_rule.try_as<op_type_expr_callback_call>()) {
    auto param = GenTree::get_call_arg_ref(callback_call_rule->expr().as<op_type_expr_arg_ref>(), extern_function_call);

    if (auto lambda_class = LambdaClassData::get_from(param)) {
      FunctionPtr template_invoke = lambda_class->get_template_of_invoke_function();
      assumption = calc_assumption_for_return(template_invoke, extern_function_call);
    }

    return tp_Unknown;
  } else if (auto index_rule = type_rule.try_as<op_index>()) {
    PrimitiveType pt = infer_type_of_callback_arg(index_rule->array(), extern_function_call, function_context, assumption);
    if (auto as_array = assumption.try_as<AssumArray>()) {
      assumption = as_array->inner;
    }
    return pt;
  } else if (auto arg_ref = type_rule.try_as<op_type_expr_arg_ref>()) {
    int id_of_call_parameter = GenTree::get_id_arg_ref(arg_ref, extern_function_call);
    kphp_assert(id_of_call_parameter != -1);

    if (id_of_call_parameter < extern_function_call->args().size()) {
      auto call_param = extern_function_call->args()[id_of_call_parameter];
      assumption = infer_class_of_expr(function_context, call_param);

      auto extern_func_params = extern_function_call->func_id->get_params();
      return extern_func_params[id_of_call_parameter]->type_help;
    }
  } else if (auto rule = type_rule.try_as<op_type_expr_type>()) {
    return rule->type_help;
  } else {
    kphp_assert(false);
  }

  return tp_Unknown;
}

std::string LambdaClassData::get_name_of_invoke_function_for_extern(VertexAdaptor<op_func_call> extern_function_call,
                                                                    FunctionPtr function_context,
                                                                    std::map<int, vk::intrusive_ptr<Assumption>> *template_type_id_to_ClassPtr /*= nullptr*/,
                                                                    FunctionPtr *template_of_invoke_method /*= nullptr*/) const {
  std::string invoke_method_name = replace_backslashes(construct_function->class_id->name) + "$$" + NAME_OF_INVOKE_METHOD;

  VertexRange call_params = extern_function_call->args();
  //int call_params_n = static_cast<int>(call_params.size());

  VertexRange extern_func_params = extern_function_call->func_id->get_params();
  auto callback_it = std::find_if(extern_func_params.begin(), extern_func_params.end(), [](VertexPtr p) { return p->type() == op_func_param_typed_callback; });
  kphp_assert(callback_it != extern_func_params.end());
  auto callback_pos = static_cast<size_t>(std::distance(extern_func_params.begin(), callback_it));
  if (auto call_func_ptr = call_params[callback_pos].try_as<op_func_ptr>()) {
    if (call_func_ptr->has_bound_class()) {
      return call_func_ptr->get_string();
    }
  }

  auto func_param_callback = callback_it->as<op_func_param_typed_callback>();
  VertexRange callback_params = get_function_params(func_param_callback);
  if (!FunctionData::check_cnt_params(callback_params.size(), *template_of_invoke_method)) {
    return "";
  }

  auto template_invoke_params = (*template_of_invoke_method)->get_params();
  for (int i = 0; i < callback_params.size(); ++i) {
    auto callback_param = callback_params[i];

    vk::intrusive_ptr<Assumption> assumption;
    auto lambda_param = template_invoke_params[i + 1].as<op_func_param>();
    if (auto type_rule = callback_param->type_rule) {
      kphp_assert(type_rule->type() == op_common_type_rule);
      lambda_param->type_help =
        infer_type_of_callback_arg(type_rule.as<op_common_type_rule>()->rule(), extern_function_call, function_context, assumption);
    }

    invoke_method_name += FunctionData::encode_template_arg_name(assumption, i + 1);
    if (!template_type_id_to_ClassPtr) {
      continue;
    }

    auto &type_id = lambda_param->template_type_id;
    if (!assumption || assumption->is_primitive()) {
      kphp_assert(lambda_param->type_help != tp_Unknown);
      type_id = -1;
    } else {
      if (type_id > -1) {
        template_type_id_to_ClassPtr->emplace(type_id, assumption);
      }
    }
  }

  return invoke_method_name;
}

VertexAdaptor<op_func_call> LambdaClassData::gen_constructor_call_pass_fields_as_args() const {
  std::vector<VertexPtr> args;
  members.for_each([&](const ClassMemberInstanceField &field) {
    VertexPtr res = VertexAdaptor<op_var>::create();
    if (field.local_name() == LambdaClassData::get_parent_this_name()) {
      res->set_string("this");
    } else {
      res->set_string(std::string{field.local_name()});
    }
    res->location = field.root->location;
    args.emplace_back(res);
  });

  return gen_constructor_call_with_args(std::move(args));
}

VertexAdaptor<op_func_call> LambdaClassData::gen_constructor_call_with_args(std::vector<VertexPtr> args) const {
  return GenTree::gen_constructor_call_with_args(get_self(), std::move(args));
}


FunctionPtr LambdaClassData::get_template_of_invoke_function() const {
  auto found_method = members.get_instance_method(NAME_OF_INVOKE_METHOD);

  return (found_method && found_method->function->is_template) ? found_method->function : FunctionPtr();
}

