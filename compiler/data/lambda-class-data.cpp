// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/data/lambda-class-data.h"

#include <string>

#include "compiler/class-assumptions.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/gentree.h"
#include "compiler/inferring/public.h"
#include "compiler/type-hint.h"
#include "compiler/utils/string-utils.h"
#include "compiler/vertex.h"

LambdaPtr LambdaClassData::get_from(VertexPtr v) {
  if (auto func_ptr = v.try_as<op_func_ptr>()) {
    if (func_ptr->func_id) {
      return get_from(func_ptr->func_id->root);
    }
  }

  if (tinf::Node *tinf_node = tinf::get_tinf_node(v)) {
    if (tinf_node->get_type()->ptype() != tp_any) {
      return tinf_node->get_type()->class_type().try_as<LambdaClassData>();
    }
  }

  if (auto function = v.try_as<op_function>()) {
    return function->func_id->class_id.try_as<LambdaClassData>();
  } else if (vk::any_of_equal(v->type(), op_func_call, op_var)) {
    ClassPtr klass = infer_class_of_expr(stage::get_function(), v).try_as_class();
    if (klass) {
      return klass.try_as<LambdaClassData>();
    }
  }

  return {};
}

void LambdaClassData::implement_interface(InterfacePtr interface) {
  implements.emplace_back(interface);
  add_virt_clone();
  add_class_constant();
  register_defines();

  auto my_invoke_method = members.get_instance_method(ClassData::NAME_OF_INVOKE_METHOD)->function;
  auto interface_invoke_method = interface->members.get_instance_method(ClassData::NAME_OF_INVOKE_METHOD)->function;
  my_invoke_method->is_template = false;

  auto my_invoke_params = my_invoke_method->get_params();
  for (size_t i = 1; i < my_invoke_params.size(); ++i) {
    auto param = my_invoke_params[i].as<op_func_param>();
    param->template_type_id = -1;
    // at first, an interface __invoke() had type hints based on phpdocs like callable(int, float[]): void
    // here we copy them for each lambda, they will be checked and auto-used for assumptions
    if (!param->type_hint) {
      param->type_hint = interface_invoke_method->get_params()[i].as<op_func_param>()->type_hint;
    }
  }

  if (!my_invoke_method->return_typehint) {
    my_invoke_method->return_typehint = interface_invoke_method->return_typehint;
  }
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

void infer_type_of_callback_arg(const TypeHint *type_hint, VertexAdaptor<op_func_call> extern_function_call,
                                FunctionPtr function_context, Assumption &assumption) {
  if (const auto *lca_rule = type_hint->try_as<TypeHintPipe>()) {
    for (const TypeHint *item : lca_rule->items) {
      infer_type_of_callback_arg(item, extern_function_call, function_context, assumption);
    }

  } else if (const auto *callback_call_rule = type_hint->try_as<TypeHintArgRefCallbackCall>()) {
    auto param = GenTree::get_call_arg_ref(callback_call_rule->arg_num, extern_function_call);
    if (auto lambda_class = LambdaClassData::get_from(param)) {
      FunctionPtr template_invoke = lambda_class->get_template_of_invoke_function();
      assumption = calc_assumption_for_return(template_invoke, extern_function_call);
    }

  } else if (const auto *subkey_get = type_hint->try_as<TypeHintArgSubkeyGet>()) {
    infer_type_of_callback_arg(subkey_get->inner, extern_function_call, function_context, assumption);
    if (Assumption inner = assumption.get_inner_if_array()) {
      assumption = inner;
    }

  } else if (const auto *arg_ref = type_hint->try_as<TypeHintArgRef>()) {
    int id_of_call_parameter = arg_ref->arg_num - 1;
    kphp_assert(id_of_call_parameter != -1);
    if (id_of_call_parameter < extern_function_call->args().size()) {
      auto call_param = extern_function_call->args()[id_of_call_parameter];
      assumption = infer_class_of_expr(function_context, call_param);
    }

  }
}

std::string LambdaClassData::get_name_of_invoke_function_for_extern(VertexAdaptor<op_func_call> extern_function_call,
                                                                    FunctionPtr function_context,
                                                                    std::map<int, Assumption> *template_type_id_to_ClassPtr /*= nullptr*/,
                                                                    FunctionPtr *template_of_invoke_method /*= nullptr*/) const {
  std::string invoke_method_name = replace_backslashes(construct_function->class_id->name) + "$$" + NAME_OF_INVOKE_METHOD;

  VertexRange call_params = extern_function_call->args();
  //int call_params_n = static_cast<int>(call_params.size());

  VertexRange extern_func_params = extern_function_call->func_id->get_params();
  auto callback_it = std::find_if(extern_func_params.begin(), extern_func_params.end(), [](VertexPtr p) { return p.as<op_func_param>()->type_hint && p.as<op_func_param>()->type_hint->try_as<TypeHintCallable>(); });
  kphp_assert(callback_it != extern_func_params.end());
  auto callback_pos = static_cast<size_t>(std::distance(extern_func_params.begin(), callback_it));
  if (auto call_func_ptr = call_params[callback_pos].try_as<op_func_ptr>()) {
    if (call_func_ptr->has_bound_class()) {
      return call_func_ptr->get_string();
    }
  }

  auto func_param_callback = callback_it->as<op_func_param>();
  const auto *type_hint_callable = func_param_callback->type_hint->try_as<TypeHintCallable>();
  if (!FunctionData::check_cnt_params(type_hint_callable->arg_types.size(), *template_of_invoke_method)) {
    return "";
  }

  auto template_invoke_params = (*template_of_invoke_method)->get_params();
  for (int i = 0; i < type_hint_callable->arg_types.size(); ++i) {
    Assumption assumption;
    auto lambda_param = template_invoke_params[i + 1].as<op_func_param>();
    if (type_hint_callable->arg_types[i]) {
      infer_type_of_callback_arg(type_hint_callable->arg_types[i], extern_function_call, function_context, assumption);
    }

    invoke_method_name += FunctionData::encode_template_arg_name(assumption, i + 1);
    if (!template_type_id_to_ClassPtr) {
      continue;
    }

    auto &type_id = lambda_param->template_type_id;
    if (!assumption.has_instance()) {
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

