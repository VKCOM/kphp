#include "compiler/data/class-data.h"

#include "compiler/data/function-data.h"
#include "compiler/utils/string-utils.h"
#include "compiler/vertex.h"

ClassData::ClassData() :
  id(0),
  class_type(ctype_class),
  assumptions_inited_vars(0),
  was_constructor_invoked(false),
  members(this) {
}

void ClassData::set_name_and_src_name(const string &name) {
  this->name = name;
  this->src_name = std::string("C$").append(replace_backslashes(name));
  this->header_name = replace_characters(src_name + ".h", '$', '@');
}

void ClassData::debugPrint() {
  string str_class_type =
    class_type == ctype_interface ? "interface" :
    class_type == ctype_trait ? "trait" :
    "class";
  printf("=== %s %s\n", str_class_type.c_str(), name.c_str());

  members.for_each([](ClassMemberConstant &m) {
    printf("const %s\n", m.local_name().c_str());
  });
  members.for_each([](ClassMemberStaticField &m) {
    printf("static $%s\n", m.local_name().c_str());
  });
  members.for_each([](ClassMemberStaticMethod &m) {
    printf("static %s()\n", m.local_name().c_str());
  });
  members.for_each([](ClassMemberInstanceField &m) {
    printf("var $%s\n", m.local_name().c_str());
  });
  members.for_each([](ClassMemberInstanceMethod &m) {
    printf("method %s()\n", m.local_name().c_str());
  });
}

PrimitiveType infer_type_of_callback_arg(VertexPtr type_rule, VertexRange call_params, FunctionPtr function_context, VertexRange extern_func_params,
                                         AssumType &assum, ClassPtr &klass_assumed) {

  if (auto func_rule = type_rule.try_as<op_type_rule_func>()) {
    if (func_rule->get_string() == "OrFalse") {
      // TODO:
      return infer_type_of_callback_arg(func_rule->args()[0], call_params, function_context, extern_func_params, assum, klass_assumed);
    } else if (func_rule->get_string() == "lca") {
      PrimitiveType result_pt = tp_Unknown;
      for (auto v : func_rule->args()) {
        PrimitiveType pt = infer_type_of_callback_arg(v, call_params, function_context, extern_func_params, assum, klass_assumed);
        if (klass_assumed) {
          return pt;
        }

        if (result_pt == tp_Unknown) {
          result_pt = pt;
        }
      }

      return result_pt != tp_Unknown ? result_pt : tp_Any;
    } else if (func_rule->get_string() == "callback_call") {
      int id_of_call_parameter = func_rule->args()[0].as<op_arg_ref>()->int_val - 1;
      kphp_assert(id_of_call_parameter >= 0 && id_of_call_parameter < static_cast<int>(call_params.size()));
      VertexPtr param = call_params[id_of_call_parameter];
      if (auto lambda_class = FunctionData::is_lambda(param)) {
        FunctionPtr template_invoke = lambda_class->get_template_of_invoke_function();
        assum = calc_assumption_for_return(template_invoke, klass_assumed);
      }

      return tp_Unknown;
    } else {
      kphp_assert(false);
    }
  } else if (auto index_rule = type_rule.try_as<op_index>()) {
    PrimitiveType pt = infer_type_of_callback_arg(index_rule->array(), call_params, function_context, extern_func_params, assum, klass_assumed);
    if (assum == assum_instance_array) {
      assum = assum_instance;
    }
    return pt;
  } else if (auto arg_ref = type_rule.try_as<op_arg_ref>()) {
    int id_of_call_parameter = arg_ref->int_val - 1;
    kphp_assert(id_of_call_parameter >= 0 && id_of_call_parameter < static_cast<int>(call_params.size()));
    assum = infer_class_of_expr(function_context, call_params[id_of_call_parameter], klass_assumed);
    return extern_func_params[id_of_call_parameter]->type_help;
  } else if (auto rule = type_rule.try_as<op_type_rule>()) {
    return rule->type_help;
  } else {
    kphp_assert(false);
  }

  return tp_Unknown;
}

std::string ClassData::get_name_of_invoke_function_for_extern(VertexAdaptor<op_func_call> extern_function_call,
                                                              FunctionPtr function_context,
                                                              std::map<int, std::pair<AssumType, ClassPtr>> *template_type_id_to_ClassPtr /*= nullptr*/,
                                                              FunctionPtr *template_of_invoke_method /*= nullptr*/) const {
  std::string invoke_method_name = replace_backslashes(new_function->class_id->name) + "$$__invoke";

  VertexRange call_params = extern_function_call->args();
  //int call_params_n = static_cast<int>(call_params.size());

  VertexRange extern_func_params = extern_function_call->get_func_id()->get_params();
  auto callback_it = std::find_if(extern_func_params.begin(), extern_func_params.end(), [](VertexPtr p) { return p->type() == op_func_param_callback; });
  kphp_assert(callback_it != extern_func_params.end());
  size_t callback_pos = static_cast<size_t>(std::distance(extern_func_params.begin(), callback_it));
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
        infer_type_of_callback_arg(type_rule.as<op_common_type_rule>()->expr(), call_params, function_context, extern_func_params, assum, klass_assumed);
    }

    switch (assum) {
      case assum_unknown:
      case assum_not_instance: {
        kphp_assert(callback_param->type_help != tp_Unknown);
        invoke_method_name += "$" + std::to_string(i + 1) + "not_instance";
        if (template_type_id_to_ClassPtr) {
          auto template_invoke_params = (*template_of_invoke_method)->get_params();
          template_invoke_params[i + 1].as<op_func_param>()->template_type_id = -1;
        }
        break;
      }

      case assum_instance_array:
        invoke_method_name += "$arr";
        /* fallthrough */
      case assum_instance: {
        invoke_method_name += "$" + replace_backslashes(klass_assumed->name);
        if (template_type_id_to_ClassPtr) {
          auto template_invoke_params = (*template_of_invoke_method)->get_params();
          auto type_id = template_invoke_params[i + 1].as<op_func_param>()->template_type_id;
          template_type_id_to_ClassPtr->emplace(type_id, std::make_pair(assum_instance, klass_assumed));
        }
        break;
      }

      default:
        break;
    }
  }

  return invoke_method_name;
}

FunctionPtr ClassData::get_invoke_function_for_extern_function(VertexAdaptor<op_func_call> extern_function_call, FunctionPtr function_context) const {
  /**
   * Will be rewritten in future
   * need finding `invoke` method which conforms to parameters types of extern_function
   */
  kphp_assert(extern_function_call->get_func_id()->is_extern);
  std::string invoke_method_name = get_name_of_invoke_function_for_extern(extern_function_call, function_context);
  auto found_method = members.find_member([&](const ClassMemberInstanceMethod &f) {
    return f.global_name() == invoke_method_name;
  });

  return found_method ? found_method->function : FunctionPtr();
}

FunctionPtr ClassData::get_template_of_invoke_function() const {
  kphp_assert(new_function->is_lambda());
  auto found_method = members.get_instance_method("__invoke");

  return (found_method && found_method->function->is_template) ? found_method->function : FunctionPtr();
}
