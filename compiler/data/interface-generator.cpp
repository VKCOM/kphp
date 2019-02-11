#include "compiler/data/interface-generator.h"

#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/gentree.h"
#include "compiler/stage.h"

namespace {

template<Operation op>
VertexAdaptor<op> create_call_with_this_and_class_name_params(ClassPtr klass) {
  auto this_var = ClassData::gen_vertex_this(0);

  auto class_name = VertexAdaptor<op_func_name>::create();
  class_name->set_string("\\" + klass->name + "::class");

  return VertexAdaptor<op>::create(this_var, class_name);
}

VertexAdaptor<op_instanceof> create_instanceof(ClassPtr derived) {
  return create_call_with_this_and_class_name_params<op_instanceof>(derived); 
}

VertexAdaptor<op_func_call> create_instance_cast_to(ClassPtr derived) {
  auto cast_to_derived = create_call_with_this_and_class_name_params<op_func_call>(derived);
  cast_to_derived->set_string("instance_cast");
  return cast_to_derived;
}

bool check_that_signatures_are_same(ClassPtr context_class, FunctionPtr interface_function) {
  const auto &interface_name = interface_function->name;
  auto interface_method_in_derived = context_class->members.get_instance_method(get_local_name_from_global_$$(interface_name));

  if (!interface_method_in_derived) {
    auto msg = format("You should override interface method: `%s` in class: `%s`",
                      interface_function->get_human_readable_name().c_str(),
                      context_class->name.c_str());

    kphp_error_act(false, msg, return false);
  }

  auto derived_method = interface_method_in_derived->function;
  kphp_assert(derived_method);

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
  derived_method->calc_min_argn();
  auto min_argn = derived_method->min_argn;
  auto max_argn = derived_method->get_params().size();
  auto default_argn = max_argn - min_argn;

  auto i_argn = interface_function->get_params().size();

  if (!(i_argn <= max_argn && (default_argn >= max_argn - i_argn))) {
    auto msg = format("Count of arguments are different in interface method: `%s` and in class: `%s`",
      interface_function->get_human_readable_name().c_str(),
      context_class->name.c_str());

    kphp_error_act(false, msg, return false);
  }

  return true;
}

VertexPtr generate_call_in_context_of_derived(ClassPtr context_class, FunctionPtr interface_function) {
  if (!check_that_signatures_are_same(context_class, interface_function)) {
    return VertexPtr{};
  }

  auto cast_to_derived = create_instance_cast_to(context_class);
  auto params = interface_function->get_params_as_vector_of_vars(1);

  auto call_method_in_context = VertexAdaptor<op_func_call>::create(cast_to_derived, params);
  call_method_in_context->set_string(get_local_name_from_global_$$(interface_function->name));
  call_method_in_context->extra_type = op_ex_func_call_arrow;

  return VertexAdaptor<op_return>::create(call_method_in_context);
}

bool check_that_function_is_abstract(FunctionPtr interface_function) {
  auto interface_class = interface_function->class_id;

  return interface_class &&
         interface_class->is_interface() &&
         interface_function->root->cmd()->empty();
}

} // anonymous namespace


/**
 * generated AST for interface function will be like this:
 *
 * function interface_function($param1, ...) {
 *   if (is_a($this, Derived_first)) {
 *     return instance_cast($this, Derived_first)->interface_function($param1, ...);
 *   } else if (is_a($this, Derived_second)) {
 *     return instance_cast($this, Derived_second)->interface_function($param1, ...);
 *   } else {
 *     return instance_cast($this, Derived_last)->interface_function($param1, ...);
 *   }
 */
void generate_body_of_interface_method(FunctionPtr interface_function) {
  auto &derived_classes = interface_function->class_id->derived_classes;

  if (derived_classes.empty()) {
    return;
  }

  kphp_assert(check_that_function_is_abstract(interface_function));

  VertexPtr body_of_interface_method;
  for (auto derived : derived_classes) {
    auto is_instance_of_derived = create_instanceof(derived);
    auto call_derived_method = generate_call_in_context_of_derived(derived, interface_function);

    if (body_of_interface_method) {
      body_of_interface_method = VertexAdaptor<op_if>::create(is_instance_of_derived, call_derived_method, body_of_interface_method);
    } else {
      body_of_interface_method = call_derived_method;
    }
  }

  auto &root = interface_function->root;
  root = VertexAdaptor<op_function>::create(root->name(), root->params(), body_of_interface_method);
  root->set_func_id(interface_function);
  interface_function->type = FunctionData::func_local;
}


