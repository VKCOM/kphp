// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/rewrite-rules/replace-extern-func-calls.h"
#include "compiler/pipes/check-access-modifiers.h"

#include <vector>

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/define-data.h"
#include "compiler/data/kphp-json-tags.h"
#include "compiler/data/function-data.h"
#include "compiler/data/kphp-tracing-tags.h"
#include "compiler/vertex-util.h"

/*
 * Sometimes, we want to replace f(...) with f'(...)
 * as f' better coincides with C++ implementation and/or C++ templates (f is extern).
 *
 * For instance, `JsonEncoder::encode()` and all inheritors' calls are replaced
 * (not to carry auto-generated inheritors body, as they are generated wrong anyway, cause JsonEncoder is built-in).
 *
 * For instance, `new $class` in PHP is stored as `_by_name_construct($class)` in AST, and here
 * this call is replaced with `new T` where T=assumption($class) for compile-time known variables.
 *
 * Note, that this file contains manually written (hardcoded) replacements that can't be expressed by DSL.
 * Replacements like `microtime(true)` => `_microtime_float()` are not listed here:
 * such simple replacements are expressed in early_opt.rules in a DSL manner, see EarlyOptimizationF.
 */

static bool is_class_JsonEncoder_or_child(ClassPtr class_id) {
  ClassPtr klass_JsonEncoder = G->get_class("JsonEncoder");
  if (klass_JsonEncoder && klass_JsonEncoder->is_parent_of(class_id)) {
    // when a class is first time detected as json encoder, parse and validate all constants, and store them
    if (!class_id->kphp_json_tags) {
      class_id->kphp_json_tags = kphp_json::convert_encoder_constants_to_tags(class_id);
    }
    return true;
  }
  return false;
}

// `new $c(...)` is stored as `_by_name_construct($c, ...)` in AST
// here we replace `_by_name_construct('A', ...args)` => `A::__construct(op_alloc, ...args)`
static VertexPtr replace_by_name_construct(VertexAdaptor<op_func_call> call) {
  const std::string *class_name = VertexUtil::get_constexpr_string(call->args()[0]);
  kphp_error_act(class_name, "Syntax 'new $class_name' works only if $class_name is compile-time known.\nIt can be achieved via generics and class-string<T>, for example.", return call);

  ClassPtr klass = G->get_class(*class_name);
  kphp_assert(klass);

  FunctionPtr m_ctor = klass->construct_function;
  if (!m_ctor) {
    kphp_error(0, fmt_format("Syntax 'new $class_name' not resolved: __construct() not found in {}", klass->as_human_readable()));
    return call;
  }

  auto v_alloc = VertexAdaptor<op_alloc>::create().set_location(call);
  v_alloc->allocated_class_name = *class_name;
  v_alloc->allocated_class = klass;

  std::vector<VertexPtr> ctor_args;
  ctor_args.reserve(call->size());
  ctor_args.emplace_back(v_alloc);
  for (int i = 1; i < call->size(); ++i) {
    ctor_args.emplace_back(call->args()[i]);
  }

  auto ctor_call = VertexAdaptor<op_func_call>::create(ctor_args).set_location(call);
  ctor_call->extra_type = op_ex_constructor_call;
  ctor_call->func_id = m_ctor;
  return ctor_call;
}

// `$c::method()` is stored as `_by_name_call_method($c, 'method')` in AST
// here we replace `_by_name_call_method('A', 'method', ...args)` => `A::method(...args)`
static VertexPtr replace_by_name_call_method(VertexAdaptor<op_func_call> call) {
  const std::string *class_name = VertexUtil::get_constexpr_string(call->args()[0]);
  const std::string *method_name = VertexUtil::get_constexpr_string(call->args()[1]);
  kphp_error_act(class_name, "Syntax '$class_name::method()' works only if $class_name is compile-time known.\nIt can be achieved via generics and class-string<T>, for example.", return call);

  ClassPtr klass = G->get_class(*class_name);
  kphp_assert(klass && method_name);

  const auto *m_method = klass->members.get_static_method(*method_name);
  if (!m_method) {
    const auto *instance_method_m = klass->get_instance_method(*method_name);
    kphp_error(!instance_method_m, fmt_format("Syntax '$class_name::method()' works only for static methods, but {} is an instance method", instance_method_m->function->as_human_readable()));
    kphp_error( instance_method_m, fmt_format("Syntax '$class_name::method()' not resolved: method {} not found in class {}", *method_name, klass->as_human_readable()));
    return call;
  }
  FunctionPtr method = m_method->function;
  kphp_error(method->is_required, fmt_format("Add @kphp-required over {}, because it's used only from a '$class_name::method()' syntax", method->as_human_readable()));

  std::vector<VertexPtr> method_args;
  method_args.reserve(call->size() - 2);
  for (int i = 2; i < call->size(); ++i) {
    method_args.emplace_back(call->args()[i]);
  }

  auto m_call = VertexAdaptor<op_func_call>::create(method_args).set_location(call);
  m_call->str_val = method->name;
  m_call->func_id = method;
  return m_call;
}

// `$c::CONST` is stored as `_by_name_get_const($c, 'CONST')` in AST
// here we replace `_by_name_get_const('A', 'const')` => `A::CONST (value of)`
static VertexPtr replace_by_name_get_const(FunctionPtr current_function, VertexAdaptor<op_func_call> call) {
  const std::string *class_name = VertexUtil::get_constexpr_string(call->args()[0]);
  const std::string *const_name = VertexUtil::get_constexpr_string(call->args()[1]);
  kphp_error_act(class_name, "Syntax '$class_name::CONST' works only if $class_name is compile-time known.\nIt can be achieved via generics and class-string<T>, for example.", return call);

  ClassPtr klass = G->get_class(*class_name);
  kphp_assert(klass && const_name);

  const auto *m_const = klass->get_constant(*const_name);
  if (!m_const) {
    kphp_error(0, fmt_format("Syntax '$class_name::CONST' not resolved: const {} not found in class {}", *const_name, klass->as_human_readable()));
    return call;
  }
  DefinePtr define = G->get_define(m_const->define_name);

  // for constants, check access modifiers right here (for other constants it was also done before)
  // (for other methods/fields, it will automatically be done later, constants are exceptions due to inlining
  ClassPtr lambda_class_id = current_function->get_this_or_topmost_if_lambda()->class_id;
  check_access(current_function->class_id, lambda_class_id, FieldModifiers{define->access}, define->class_id, "const", define->as_human_readable());

  return define->val.clone().set_location_recursively(call);
}

// `$c::$field` is stored as `_by_name_get_field($c, 'field')` in AST
// here we replace `_by_name_get_field('A', 'field')` => `A::$field`
static VertexPtr replace_by_name_get_field(VertexAdaptor<op_func_call> call) {
  const std::string *class_name = VertexUtil::get_constexpr_string(call->args()[0]);
  const std::string *field_name = VertexUtil::get_constexpr_string(call->args()[1]);
  kphp_error_act(class_name, "Syntax '$class_name::$field' works only if $class_name is compile-time known.\nIt can be achieved via generics and class-string<T>, for example.", return call);

  ClassPtr klass = G->get_class(*class_name);
  kphp_assert(klass && field_name);

  const auto *m_field = klass->get_static_field(*field_name);
  if (!m_field) {
    const auto *instance_field_m = klass->get_instance_field(*field_name);
    kphp_error(!instance_field_m, fmt_format("Syntax '$class_name::$field' works only for static fields, but {} is an instance field", instance_field_m->var->as_human_readable()));
    kphp_error( instance_field_m, fmt_format("Syntax '$class_name::$field' not resolved: field ${} not found in class {}", *field_name, klass->as_human_readable()));
    return call;
  }
  VarPtr field = m_field->var;

  auto f_get = VertexAdaptor<op_var>::create().set_location(call);
  f_get->str_val = field->name;
  return f_get;
}

// `JsonEncoderOrChild::decode($json, $class_name)` => `JsonEncoder::from_json_impl('JsonEncoderOrChild', $json, $class_name)`
static VertexPtr replace_JsonEncoder_decode(VertexAdaptor<op_func_call> call) {
  auto v_encoder_name = VertexAdaptor<op_string>::create().set_location(call->location);
  v_encoder_name->str_val = call->func_id->class_id->name;
  call->str_val = "JsonEncoder$$from_json_impl";
  call->func_id = G->get_function(call->str_val);
  return VertexUtil::add_call_arg(v_encoder_name, call, true);
}

// `JsonEncoderOrChild::encode($instance)` => `JsonEncoder::to_json_impl('JsonEncoderOrChild', $instance)`
static VertexPtr replace_JsonEncoder_encode(VertexAdaptor<op_func_call> call) {
  auto v_encoder_name = VertexAdaptor<op_string>::create().set_location(call->location);
  v_encoder_name->str_val = call->func_id->class_id->name;
  call->str_val = "JsonEncoder$$to_json_impl";
  call->func_id = G->get_function(call->str_val);
  return VertexUtil::add_call_arg(v_encoder_name, call, true);
}

// `JsonEncoderOrChild::getLastError()` => `JsonEncoder::getLastError()`
static VertexPtr replace_JsonEncoder_getLastError(VertexAdaptor<op_func_call> call) {
  call->str_val = "JsonEncoder$$getLastError";
  call->func_id = G->get_function(call->str_val);
  return call;
}

// `classof(expr)` should be replaced in advance: it's not a regular function, it's available only in f<T>
static VertexPtr replace_classof([[maybe_unused]] FunctionPtr current_function, VertexAdaptor<op_func_call> call) {
  kphp_error(0, "classof() used incorrectly: it's a keyword to be used only for a single variable which is a generic parameter");
  return call;
}


// it's the main (exported) function
// see comment at the top of the file
VertexPtr maybe_replace_extern_func_call(FunctionPtr current_function, VertexAdaptor<op_func_call> call) {
  FunctionPtr f_called = call->func_id;
  const std::string &f_name = f_called->name;
  int n_args = call->size();

  // new $c, $c::method(), $c::CONST, $c::$FIELD
  // see GenTree::get_member_by_name_after_var()
  if (f_name == "_by_name_construct") {
    return replace_by_name_construct(call);
  }
  if (f_name == "_by_name_call_method") {
    return replace_by_name_call_method(call);
  }
  if (f_name == "_by_name_get_const") {
    return replace_by_name_get_const(current_function, call);
  }
  if (f_name == "_by_name_get_field") {
    return replace_by_name_get_field(call);
  }

  // JsonEncoder::encode(), MyJsonEncoder::decode(), and similar
  if (f_called->modifiers.is_static() && f_called->class_id) {
    vk::string_view local_name = f_called->local_name();

    if (local_name == "decode" && is_class_JsonEncoder_or_child(f_called->class_id) && n_args >= 2) {
      return replace_JsonEncoder_decode(call.clone());
    }
    if (local_name == "encode" && is_class_JsonEncoder_or_child(f_called->class_id) && n_args >= 1) {
      return replace_JsonEncoder_encode(call.clone());
    }
    if (local_name == "getLastError" && is_class_JsonEncoder_or_child(f_called->class_id) && n_args == 0) {
      return replace_JsonEncoder_getLastError(call.clone());
    }
  }

  // classof($expr)
  if (f_name == "classof" && n_args == 1) {
    return replace_classof(current_function, call);
  }

  // register_shutdown_function(f_shutdown) — automatically mark f_shutdown with @kphp-tracing
  // (it's also a kind of "replace when extern", that's why placed here, but can be moved to another place)
  if (f_name == "register_shutdown_function" && n_args >= 1) {
    FunctionPtr f_shutdown = call->args()[0].as<op_callback_of_builtin>()->func_id;
    if (f_shutdown && !f_shutdown->kphp_tracing) {
      f_shutdown->kphp_tracing = KphpTracingDeclarationMixin::create_for_shutdown_function(f_shutdown);
    }
  }

  return call;
}
