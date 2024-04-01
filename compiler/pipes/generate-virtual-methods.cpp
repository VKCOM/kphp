// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/generate-virtual-methods.h"

#include "common/algorithms/contains.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/type-hint.h"
#include "compiler/vertex-util.h"

// a "virtual method" is an instance method overridden in child classes (all methods of interfaces are virtual also)
// $obj->virtual_method() call actually dynamically handles the runtime type of $obj, proxying a call to a concrete class
// the main function here is generate_body_of_virtual_method() that deals with virtual dispatching
// it's implemented below, see commnents above it

// also, here we perform some checks for inheritance
// for instance, types in arguments/returns should be contravariant/covariant, as well as compatible with PHP when used as type hints

namespace {

VertexAdaptor<op_func_call> generate_instance_cast_to_call(VertexAdaptor<op_var> instance_var, ClassPtr derived) {
  auto class_name = VertexAdaptor<op_string>::create();
  class_name->str_val = derived->name;

  auto call_instance_cast = VertexAdaptor<op_func_call>::create(instance_var, class_name);
  call_instance_cast->str_val = "instance_cast";
  call_instance_cast->func_id = G->get_function(call_instance_cast->str_val);

  return call_instance_cast;
}

VertexAdaptor<op_func_call> generate_call_of_self_method(VertexPtr instance_var, FunctionPtr function, FunctionPtr self_method) {
  auto params = function->get_params_as_vector_of_vars(1);
  auto call_method = VertexAdaptor<op_func_call>::create(instance_var, params);

  if (function->has_variadic_param) {
    kphp_assert(!call_method->args().empty());
    auto &last_param_passed_to_method = *std::prev(call_method->args().end());
    last_param_passed_to_method = VertexAdaptor<op_varg>::create(params.back()).set_location(params.back());
  }

  call_method->str_val = std::string{self_method->local_name()};
  call_method->extra_type = op_ex_func_call_arrow;
  call_method->func_id = self_method;

  return call_method;
}

VertexAdaptor<op_func_call> generate_critical_error_call(std::string msg) {
  auto msg_v = VertexAdaptor<op_string>::create();
  msg_v->str_val = std::move(msg);

  auto call_ce = VertexAdaptor<op_func_call>::create(msg_v);
  call_ce->str_val = "critical_error";
  call_ce->func_id = G->get_function(call_ce->str_val);

  return call_ce;
}

enum class VarianceKind {
  ReturnTypeHints, // covariance
  ParamTypeHints,  // contravariance
};

// check that type hints are compatible following the PHP rules
//
// ReturnTypeHints: derived type can be more specific than the base type
// ParamTypeHints: derived type can be less specific than the base type
// see https://www.php.net/manual/en/language.oop5.variance.php
//
// note: the PHP docs lie about int<->float, int is not considered to be a more specific type
bool php_type_hints_compatible(VarianceKind mode, const TypeHint *base, const TypeHint *derived) {
  if (mode == VarianceKind::ParamTypeHints) {
    return php_type_hints_compatible(VarianceKind::ReturnTypeHints, derived, base);
  }

  // no base type hint -> anything is allowed
  // identical type hint -> OK
  if (!base || base == derived) {
    return true;
  }
  if (!derived) {
    return false;
  }

  if (const auto *base_array = base->try_as<TypeHintArray>()) {
    if (const auto *derived_array = derived->try_as<TypeHintArray>()) {
      return php_type_hints_compatible(mode, base_array->inner, derived_array->inner);
    }
  }

  if (const auto *base_optional = base->try_as<TypeHintOptional>()) {
    if (!base_optional->or_false && base_optional->or_null) {
      if (const auto *derived_optional = derived->try_as<TypeHintOptional>()) {
        if (!derived_optional->or_false && derived_optional->or_null) {
          return php_type_hints_compatible(mode, base_optional->inner, derived_optional->inner);
        }
      }
      // covariance allows base `?T` type to be compared as `T`, but not the other way around
      return base_optional->or_null && !base_optional->or_false && php_type_hints_compatible(mode, base_optional->inner, derived);
    }
    // or-false types are handled below
  }

  // can't use Assumption::extract_instance_from_type_hint as it will
  // unwrap the TypeHintOptional here
  if (const auto *base_instance = base->try_as<TypeHintInstance>()) {
    if (const auto *derived_instance = derived->try_as<TypeHintInstance>()) {
      return base_instance->resolve()->is_parent_of(derived_instance->resolve());
    }
    // TODO: this should not be allowed, `T -> null` is not a valid transition;
    // we could probably use ?T instead of standalone null in this case
    // see tests/phpt/interfaces/signature_compatibility return_null.php
    if (const auto *derived_primitive = derived->try_as<TypeHintPrimitive>()) {
      return derived_primitive->ptype == tp_Null;
    }
    return false;
  }

  if (const auto *base_callable = base->try_as<TypeHintCallable>()) {
    if (const auto *derived_callable = derived->try_as<TypeHintCallable>()) {
      if (base_callable->is_untyped_callable() && derived_callable->is_untyped_callable()) {
        return true;
      }
      // for typed callables inheritance, we allow when parent/child are totally equal
      // we don't allow co[ntra]variance in typed callables' params/return
      if (base_callable->is_typed_callable() && derived_callable->is_typed_callable()
          && base_callable->arg_types.size() == derived_callable->arg_types.size()) {
        for (int i = 0; i < base_callable->arg_types.size(); ++i) {
          if (base_callable->arg_types[i] != derived_callable->arg_types[i]) {
            return false;
          }
        }
        return base_callable->return_type == derived_callable->return_type;
      }
    }
    return false;
  }

  // this is not compliant to the PHP behavior as we would allow
  // a mixture of incompatible unions here, but this can be improved in the future;
  //
  // we'll probably want to sort (by hash?) all TypeHintPipe items to make
  // their full comparison faster (without a map or O(n^2) complexity)
  if (base->is_typedata_constexpr() && derived->is_typedata_constexpr()) {
    const auto *base_td = base->to_type_data();
    const auto *derived_td = derived->to_type_data();
    if (base_td->ptype() == tp_float && derived_td->ptype() == tp_int) {
      return false;
    }
    return is_less_or_equal_type(derived_td, base_td);
  }

  return false;
}

bool check_php_signatures_variance(FunctionPtr base_function, FunctionPtr derived_method, std::string &error_info) {
  const auto type_hint_string = [](const TypeHint *type_hint) { return type_hint ? type_hint->as_human_readable() : "<none>"; };

  if (!php_type_hints_compatible(VarianceKind::ReturnTypeHints, base_function->return_typehint, derived_method->return_typehint)) {
    error_info = fmt_format("\t{} return type: {}\n", base_function->class_id->name, type_hint_string(base_function->return_typehint))
                 + fmt_format("\t{} return type: {}\n", derived_method->class_id->name, type_hint_string(derived_method->return_typehint))
                 + "\tNote: derived method return type can't be more generic, but it can be more specific (it should be covariant)";
    return false;
  }

  auto derived_params = derived_method->get_params();
  auto base_params = base_function->get_params();
  int first_param = base_function->modifiers.is_static() ? 0 : 1;
  for (int i = first_param; i < base_params.size(); i++) {
    auto base_param = base_params[i].as<op_func_param>();
    auto derived_param = derived_params[i].as<op_func_param>();
    if (!php_type_hints_compatible(VarianceKind::ParamTypeHints, base_param->type_hint, derived_param->type_hint)) {
      error_info =
        fmt_format("\t{} parameter ${} type: {}\n", base_function->class_id->name, base_param->var()->get_string(), type_hint_string(base_param->type_hint))
        + fmt_format("\t{} parameter ${} type: {}\n", derived_method->class_id->name, derived_param->var()->get_string(),
                     type_hint_string(derived_param->type_hint))
        + "\tNote: derived method param type can't be more specific, but it can be more generic (it should be contravariant)";
      return false;
    }
  }

  return true;
}

template<class ClassMemberMethod>
bool check_that_signatures_are_same(FunctionPtr interface_function, ClassPtr context_class, const ClassMemberMethod *interface_method_in_derived) {
  stage::set_line(interface_function->root->get_location().line);
  if (!interface_method_in_derived || (interface_method_in_derived->function->is_constructor() && !context_class->has_custom_constructor)) {
    kphp_error_act(context_class->modifiers.is_abstract(),
                   fmt_format("class {} must be abstract: method {} is not overridden", context_class->name, interface_function->as_human_readable()),
                   return true);
    for (auto derived : context_class->derived_classes) {
      const auto *method_in_derived = derived->members.find_by_local_name<ClassMemberMethod>(interface_function->local_name());
      if (!check_that_signatures_are_same(interface_function, derived, method_in_derived)) {
        return false;
      }
    }
    return true;
  }

  FunctionPtr derived_method = interface_method_in_derived->function;

  /**
   * Check that overridden method has count of parameters at least as in interface
   * and has appropriate count of default parameters e.g.:
   *
   * interface I { public function foo($x, $y); }
   * class A implements I { public function foo($x, $y = 10, $z = 20); }
   *
   * Interface I: i_argn = 2
   * Class A    : min_argn = 1, max_argn = 3, default_argn = 2
   * check that : i_argn <= max_argn && (default_argn >= max_argn - i_argn)
   */
  auto min_argn = derived_method->get_min_argn();
  auto derived_params = derived_method->get_params();
  auto max_argn = derived_params.size();
  auto default_argn = max_argn - min_argn;

  auto interface_params = interface_function->get_params();
  auto i_argn = interface_params.size();

  if (!(i_argn <= max_argn && (default_argn >= max_argn - i_argn))) {
    stage::set_location(derived_method->root->location);
    kphp_error(false, fmt_format("Count of arguments differ in {} and {}", interface_function->as_human_readable(), derived_method->as_human_readable()));
    return false;
  }

  // non-required methods may not have enough types information to be checked here;
  // we skip them to avoid false positives
  if (derived_method->is_required && !derived_method->is_invoke_method()) {
    std::string error_info;
    if (!check_php_signatures_variance(interface_function, derived_method, error_info)) {
      stage::set_location(derived_method->root->location);
      kphp_error(false, fmt_format("Declaration of {}() must be compatible with {}()\n{}", derived_method->as_human_readable(),
                                   interface_function->as_human_readable(), error_info));
    }
  }

  return true;
}

bool check_that_signatures_are_same(ClassPtr context_class, FunctionPtr interface_function) {
  const auto *interface_method_in_derived = context_class->members.get_instance_method(interface_function->local_name());
  return check_that_signatures_are_same(interface_function, context_class, interface_method_in_derived);
}

void check_static_function_signatures_compatibility(FunctionPtr interface_function, const std::vector<ClassPtr> &derived_classes) {
  for (ClassPtr derived : derived_classes) {
    const auto *static_in_derived = derived->members.get_static_method(interface_function->local_name());
    check_that_signatures_are_same(interface_function, derived, static_in_derived);
  }
}

void check_constructor_signature_compatibility(FunctionPtr interface_constructor, const std::vector<ClassPtr> &derived_classes) {
  for (ClassPtr derived : derived_classes) {
    const auto *derived_constructor = derived->members.get_instance_method(ClassData::NAME_OF_CONSTRUCT);
    check_that_signatures_are_same(interface_constructor, derived, derived_constructor);
  }
}

VertexAdaptor<op_case> gen_case_on_hash(ClassPtr derived, VertexAdaptor<op_seq> cmd) {
  auto hash_of_derived = VertexUtil::create_int_const(derived->get_hash());
  return VertexAdaptor<op_case>::create(hash_of_derived, cmd);
}

VertexAdaptor<op_case> gen_case_calling_methods_on_derived_class(ClassPtr derived, FunctionPtr virtual_function) {
  FunctionPtr concrete_method_of_derived;
  if (const auto *method_of_derived = derived->members.get_instance_method(virtual_function->local_name())) {
    concrete_method_of_derived = method_of_derived->function;
  } else {
    if (const auto *method_from_ancestor = derived->get_instance_method(virtual_function->local_name())) {
      concrete_method_of_derived = method_from_ancestor->function;
    }

    bool is_overridden_by_ancestors = concrete_method_of_derived && !concrete_method_of_derived->modifiers.is_abstract();
    if (virtual_function->modifiers.is_abstract() && !derived->modifiers.is_abstract() && !is_overridden_by_ancestors) {
      kphp_error(false, fmt_format("You should override abstract method {} in class {}", virtual_function->as_human_readable(), derived->as_human_readable()));
      return {};
    }
  }

  if (!concrete_method_of_derived || concrete_method_of_derived->modifiers.is_abstract()) {
    return {};
  }

  if (!concrete_method_of_derived->file_id->is_builtin() && concrete_method_of_derived->is_virtual_method) {
    auto &members_of_derived_class = concrete_method_of_derived->class_id->members;
    concrete_method_of_derived = members_of_derived_class.get_instance_method(virtual_function->get_name_of_self_method())->function;
  }

  if (!check_that_signatures_are_same(concrete_method_of_derived->class_id, virtual_function)) {
    return {};
  }

  VertexPtr this_var = generate_instance_cast_to_call(ClassData::gen_vertex_this({}), derived);
  // generate concrete_method call, with arguments from virtual_functions, because of Derived can have extra default params:
  auto call_self_method_of_derived = generate_call_of_self_method(this_var, virtual_function, concrete_method_of_derived);
  auto call_derived_method = VertexAdaptor<op_seq>::create(VertexAdaptor<op_return>::create(call_self_method_of_derived));

  return gen_case_on_hash(derived, call_derived_method);
}

// we can't express "return default of ReturnT", something like 'return {}' in C++ code
// that's why, we accept most common cases — "return 0" for int, "return null" for instances, etc.
VertexPtr generate_default_value_of_type(const TypeHint *ret_type) {
  if (const auto *as_optional = ret_type->try_as<TypeHintOptional>()) {
    return as_optional->or_null ? VertexPtr(VertexAdaptor<op_null>::create()) : VertexPtr(VertexAdaptor<op_false>::create());
  } else if (ret_type->try_as<TypeHintInstance>()) {
    return VertexAdaptor<op_null>::create();
  } else if (ret_type->try_as<TypeHintArray>()) {
    return VertexAdaptor<op_array>::create();
  } else if (const auto *as_tuple = ret_type->try_as<TypeHintTuple>()) {
    std::vector<VertexPtr> args;
    for (const TypeHint *item : as_tuple->items) {
      VertexPtr arg = generate_default_value_of_type(item);
      if (!arg) {
        return {};
      }
      args.emplace_back(arg);
    }
    return VertexAdaptor<op_tuple>::create(args);
  } else if (const auto *ret_primitive = ret_type->try_as<TypeHintPrimitive>()) {
    switch (ret_primitive->ptype) {
      case tp_int:
      case tp_float:
      case tp_mixed:
        return VertexUtil::create_int_const(0);
      case tp_bool:
        return VertexAdaptor<op_false>::create();
      case tp_string:
        return VertexAdaptor<op_string>::create();
      default:
        break;
    }
  } else if (ret_type->try_as<TypeHintFFIType>()) {
    return VertexAdaptor<op_null>::create();
  }
  return {};
}

// when there are no implementations of an interface, we could generate an empty body for a method
// that is just { return defaultT(); } of a return type
// it won't be executed at runtime, but it would be compiled by gcc
// this could be done if all type hints of params/return are set strictly (otherwise, Unknown would be left after inferring)
VertexAdaptor<op_seq> generate_body_of_empty_virtual_method(ClassPtr klass, FunctionPtr method) {
  const TypeHint *ret_type = method->return_typehint ?: TypeHintPrimitive::create(tp_void);
  bool all_type_hints_set = true;
  bool any_type_hint_has_tp_any = ret_type->has_tp_any_inside();
  for (auto p : method->get_params()) {
    auto param = p.as<op_func_param>();
    all_type_hints_set &= param->type_hint != nullptr;
    any_type_hint_has_tp_any |= param->type_hint != nullptr && param->type_hint->has_tp_any_inside();
  }

  kphp_error_act(all_type_hints_set,
                 fmt_format("Error compiling {}, because {} has no inheritors.\nProvide types for all params (via type hints or phpdoc)",
                            method->as_human_readable(), klass->as_human_readable()),
                 return VertexAdaptor<op_seq>::create());
  kphp_error_act(!any_type_hint_has_tp_any,
                 fmt_format("Error compiling {}, because {} has no inheritors.\nTypes must not contain 'any' or 'array', they should be definite",
                            method->as_human_readable(), klass->as_human_readable()),
                 return VertexAdaptor<op_seq>::create());

  if (ret_type->try_as<TypeHintPrimitive>() && ret_type->try_as<TypeHintPrimitive>()->ptype == tp_void) {
    return VertexAdaptor<op_seq>::create(VertexAdaptor<op_return>::create());
  }

  VertexPtr return_expr = generate_default_value_of_type(ret_type);
  kphp_error_act(return_expr,
                 fmt_format("Error compiling {}, because {} has no inheritors.\nKPHP can't generate a default implementation, because the return type is too "
                            "complex",
                            method->as_human_readable(), klass->as_human_readable()),
                 return VertexAdaptor<op_seq>::create());

  return VertexAdaptor<op_seq>::create(VertexAdaptor<op_return>::create(return_expr));
}

/**
 * generated AST for virtual methods will be like this:
 *
 * function virtual_function($param1, ...) {
 *   switch ($this->get_virt_hash()) {
 *   case 0xDEADBEAF: { // hash of Derived1
 *     $tmp = instance_cast<Derived1>($this);
 *     $tmp->virtual_function($param1, ...);
 *     break;
 *   }
 *   case 0x02280228: { // hash of Derived2
 *     $tmp = instance_cast<Derived2>($this);
 *     $tmp->virtual_function($param1, ...);
 *     break;
 *   }
 *   default: {
 *     critical_error("call method(Interface::virtual_function) on null object
 *   }
 */
void generate_body_of_virtual_method(FunctionPtr virtual_function) {
  auto klass = virtual_function->class_id;
  kphp_assert(klass);

  // if Base::f() and Derived::f(), then Base::f() is virtual, body of f() moved to f_self$()
  if (!virtual_function->modifiers.is_abstract()) {
    kphp_assert(klass->members.has_instance_method(virtual_function->get_name_of_self_method()));
    kphp_assert(virtual_function->root->cmd()->empty());
  }

  std::vector<VertexPtr> cases;
  std::vector<ClassPtr> all_derived_classes = klass->get_all_derived_classes();
  std::sort(all_derived_classes.begin(), all_derived_classes.end());
  ClassPtr prev_derived;

  for (ClassPtr derived : all_derived_classes) {
    kphp_error(prev_derived != derived,
               fmt_format("Duplicated class {} in hierarchy from class {}.\nDiamond inheritance is not supported", derived->name, klass->name));
    prev_derived = derived;

    if (auto v_case = gen_case_calling_methods_on_derived_class(derived, virtual_function)) {
      cases.emplace_back(v_case);
    }
  }
  if (!cases.empty()) {
    auto case_default_warn = generate_critical_error_call(fmt_format("call method({}) on null object", virtual_function->as_human_readable()));
    cases.emplace_back(VertexAdaptor<op_default>::create(VertexAdaptor<op_seq>::create(case_default_warn)));
  }

  if (cases.empty() && !stage::has_error()) {
    // when there are no inheritors of an interface, generate an empty body if possible —
    // it won't be executed at runtime, but it would be compiled by gcc
    virtual_function->root->cmd_ref() = generate_body_of_empty_virtual_method(klass, virtual_function);
  } else {
    // create a dispatching switch through all inheritors
    auto call_get_hash = VertexAdaptor<op_func_call>::create(ClassData::gen_vertex_this({}));
    call_get_hash->str_val = "get_hash_of_class";
    call_get_hash->func_id = G->get_function(call_get_hash->str_val);
    virtual_function->root->cmd_ref() = VertexAdaptor<op_seq>::create(VertexUtil::create_switch_vertex(virtual_function, call_get_hash, std::move(cases)));
  }

  virtual_function->type = FunctionData::func_local; // could be func_extern before, but now it has a body
  virtual_function->update_location_in_body();
}

} // namespace

void GenerateVirtualMethodsF::on_finish(DataStream<FunctionPtr> &os) {
  stage::set_name("Generate virtual method dispatching");
  stage::die_if_global_errors();

  std::forward_list<FunctionPtr> all_functions = tmp_stream.flush();

  // this loop takes about 0.4 sec in one thread; in the future, it can be parallelized
  for (FunctionPtr f : all_functions) {
    kphp_assert(!f->is_generic());

    if (f->modifiers.is_static() && f->modifiers.is_abstract()) { // is_abstract() to make vkcom compile, as it has invalid phpdoc inheritance
      check_static_function_signatures_compatibility(f, f->class_id->derived_classes);
    }
    if (f->is_virtual_method) {
      if (f->is_constructor()) {
        check_constructor_signature_compatibility(f, f->class_id->derived_classes);
      } else {
        stage::set_location(f->root->location);
        generate_body_of_virtual_method(f);
      }
    }
  }

  // not 100% sure that it's safe to do "os << f" inside the loop above, before all virtual bodies are generated
  for (FunctionPtr f : all_functions) {
    os << f;
  }
}
