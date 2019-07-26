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
  if (v->type() == op_func_ptr && v->get_func_id()) {
    return get_from(v->get_func_id()->root);
  }

  if (tinf::Node *tinf_node = tinf::get_tinf_node(v)) {
    if (tinf_node->get_type()->ptype() != tp_Unknown) {
      return tinf_node->get_type()->class_type().try_as<LambdaClassData>();
    }
  }

  if (vk::any_of_equal(v->type(), op_function, op_constructor_call) && v->get_func_id()->class_id) {
    return v->get_func_id()->class_id.try_as<LambdaClassData>();
  } else if (vk::any_of_equal(v->type(), op_func_call, op_var)) {
    ClassPtr c;
    if (infer_class_of_expr(stage::get_function(), v, c) == assum_instance) {
      return c.try_as<LambdaClassData>();
    }
  }

  return {};
}

void LambdaClassData::infer_uses_assumptions(FunctionPtr parent_function) {
  members.for_each([=](const ClassMemberInstanceField &field) {
    AssumType assum = AssumType::assum_unknown;
    ClassPtr inferred_class;
    std::string local_name = field.local_name();
    if (local_name == LambdaClassData::get_parent_this_name()) {
      if (parent_function->is_lambda()) {
        assum = assumption_get_for_var(parent_function->class_id, LambdaClassData::get_parent_this_name(), inferred_class);
      } else {
        local_name = "this";
      }
    }
    if (assum == AssumType::assum_unknown) {
      assum = calc_assumption_for_var(parent_function, local_name, inferred_class);
    }
    assumptions_for_vars.emplace_back(assum, field.local_name(), inferred_class);
  });
}

PrimitiveType infer_type_of_callback_arg(VertexPtr type_rule, VertexAdaptor<op_func_call> extern_function_call,
                                         FunctionPtr function_context, AssumType &assum, ClassPtr &klass_assumed) {
  if (auto or_false_rule = type_rule.try_as<op_type_expr_or_false>()) {
    // TODO:
    return infer_type_of_callback_arg(or_false_rule->expr(), extern_function_call, function_context, assum, klass_assumed);
  } else if (auto lca_rule = type_rule.try_as<op_type_expr_lca>()) {
    PrimitiveType result_pt = tp_Unknown;
    for (auto v : lca_rule->args()) {
      PrimitiveType pt = infer_type_of_callback_arg(v, extern_function_call, function_context, assum, klass_assumed);
      if (klass_assumed) {
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
      assum = calc_assumption_for_return(template_invoke, extern_function_call, klass_assumed);
    }

    return tp_Unknown;
  } else if (auto index_rule = type_rule.try_as<op_index>()) {
    PrimitiveType pt = infer_type_of_callback_arg(index_rule->array(), extern_function_call, function_context, assum, klass_assumed);
    if (assum == assum_instance_array) {
      assum = assum_instance;
    }
    return pt;
  } else if (auto arg_ref = type_rule.try_as<op_type_expr_arg_ref>()) {
    int id_of_call_parameter = GenTree::get_id_arg_ref(arg_ref, extern_function_call);
    kphp_assert(id_of_call_parameter != -1);

    auto call_param = extern_function_call->args()[id_of_call_parameter];
    assum = infer_class_of_expr(function_context, call_param, klass_assumed);

    auto extern_func_params = extern_function_call->get_func_id()->get_params();
    return extern_func_params[id_of_call_parameter]->type_help;
  } else if (auto rule = type_rule.try_as<op_type_expr_type>()) {
    return rule->type_help;
  } else {
    kphp_assert(false);
  }

  return tp_Unknown;
}

std::string LambdaClassData::get_name_of_invoke_function_for_extern(VertexAdaptor<op_func_call> extern_function_call,
                                                              FunctionPtr function_context,
                                                              std::map<int, std::pair<AssumType, ClassPtr>> *template_type_id_to_ClassPtr /*= nullptr*/,
                                                              FunctionPtr *template_of_invoke_method /*= nullptr*/) const {
  std::string invoke_method_name = replace_backslashes(construct_function->class_id->name) + "$$__invoke";

  VertexRange call_params = extern_function_call->args();
  //int call_params_n = static_cast<int>(call_params.size());

  VertexRange extern_func_params = extern_function_call->get_func_id()->get_params();
  auto callback_it = std::find_if(extern_func_params.begin(), extern_func_params.end(), [](VertexPtr p) { return p->type() == op_func_param_callback; });
  kphp_assert(callback_it != extern_func_params.end());
  auto callback_pos = static_cast<size_t>(std::distance(extern_func_params.begin(), callback_it));
  if (auto call_func_ptr = call_params[callback_pos].try_as<op_func_ptr>()) {
    if (call_func_ptr->has_bound_class()) {
      return call_func_ptr->get_string();
    }
  }

  auto func_param_callback = callback_it->as<op_func_param_callback>();
  VertexRange callback_params = get_function_params(func_param_callback);
  for (int i = 0; i < callback_params.size(); ++i) {
    auto callback_param = callback_params[i];

    AssumType assum = assum_not_instance;
    ClassPtr klass_assumed;
    if (auto type_rule = callback_param->type_rule) {
      kphp_assert(type_rule->type() == op_common_type_rule);
      callback_param->type_help =
        infer_type_of_callback_arg(type_rule.as<op_common_type_rule>()->rule(), extern_function_call, function_context, assum, klass_assumed);
    }

    invoke_method_name += FunctionData::encode_template_arg_name(assum, i + 1, klass_assumed);
    if (!template_type_id_to_ClassPtr) {
      continue;
    }

    auto template_invoke_params = (*template_of_invoke_method)->get_params();
    auto &type_id = template_invoke_params[i + 1].as<op_func_param>()->template_type_id;
    switch (assum) {
      case assum_unknown:
      case assum_not_instance: {
        kphp_assert(callback_param->type_help != tp_Unknown);
        type_id = -1;
        break;
      }

      case assum_instance_array:
      case assum_instance: {
        if (type_id > -1) {
          template_type_id_to_ClassPtr->emplace(type_id, std::make_pair(assum, klass_assumed));
        }
        break;
      }
    }
  }

  return invoke_method_name;
}

VertexPtr LambdaClassData::gen_constructor_call_pass_fields_as_args() const {
  std::vector<VertexPtr> args;
  members.for_each([&](const ClassMemberInstanceField &field) {
    VertexPtr res = VertexAdaptor<op_var>::create();
    if (field.local_name() == LambdaClassData::get_parent_this_name()) {
      res->set_string("this");
    } else {
      res->set_string(field.local_name());
    }
    res->location = field.root->location;
    args.emplace_back(res);
  });

  return gen_constructor_call_with_args(std::move(args));
}

VertexAdaptor<op_constructor_call> LambdaClassData::gen_constructor_call_with_args(std::vector<VertexPtr> args) const {
  args.insert(args.begin(), VertexAdaptor<op_false>::create());
  auto constructor_call = VertexAdaptor<op_constructor_call>::create(std::move(args));
  constructor_call->set_string(name);
  constructor_call->set_func_id(construct_function);

  return constructor_call;
}


FunctionPtr LambdaClassData::get_template_of_invoke_function() const {
  auto found_method = members.get_instance_method("__invoke");

  return (found_method && found_method->function->is_template) ? found_method->function : FunctionPtr();
}

