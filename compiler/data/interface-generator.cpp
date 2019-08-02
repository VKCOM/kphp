#include "compiler/data/interface-generator.h"

#include "common/termformat/termformat.h"

#include "compiler/const-manipulations.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/gentree.h"
#include "compiler/stage.h"

namespace {

template<Operation op>
VertexAdaptor<op> create_call_with_var_and_class_name_params(VertexAdaptor<op_var> instance_var, ClassPtr klass) {
  auto class_name = VertexAdaptor<op_func_name>::create();
  class_name->set_string("\\" + klass->name + "::class");

  return VertexAdaptor<op>::create(instance_var, class_name);
}

VertexAdaptor<op_instanceof> create_instanceof(VertexAdaptor<op_var> instance_var, ClassPtr derived) {
  return create_call_with_var_and_class_name_params<op_instanceof>(instance_var, derived);
}

VertexAdaptor<op_func_call> create_instance_cast_to(VertexAdaptor<op_var> instance_var, ClassPtr derived) {
  auto cast_to_derived = create_call_with_var_and_class_name_params<op_func_call>(instance_var, derived);
  cast_to_derived->set_string("instance_cast");
  return cast_to_derived;
}

template<class ClassMemberMethod>
bool check_that_signatures_are_same(FunctionPtr interface_function, ClassPtr context_class, ClassMemberMethod *interface_method_in_derived) {
  stage::set_line(get_location(interface_function->root).line);
  if (!interface_method_in_derived) {
    auto msg = format("You should override interface method: `%s` in class: `%s`",
                      interface_function->get_human_readable_name().c_str(),
                      context_class->name.c_str());

    kphp_error(false, msg);
    return false;
  }

  FunctionPtr derived_method = interface_method_in_derived->function;
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
  auto min_argn = derived_method->get_min_argn();
  auto derived_params = derived_method->get_params();
  auto max_argn = derived_params.size();
  auto default_argn = max_argn - min_argn;

  auto interface_params = interface_function->get_params();
  auto i_argn = interface_params.size();

  if (!(i_argn <= max_argn && (default_argn >= max_argn - i_argn))) {
    auto msg = format("Count of arguments are different in interface method: `%s` and in class: `%s`",
                      interface_function->get_human_readable_name().c_str(),
                      context_class->name.c_str());

    kphp_error_act(false, msg, return false);
  }

  auto get_string_repr = [] (VertexPtr v) {
    auto param = v.try_as<op_func_param>();
    kphp_assert(param && param->has_default_value());
    return VertexPtrFormatter::to_string(param->default_value());
  };

  for (auto arg_id = interface_function->get_min_argn(); arg_id < i_argn; ++arg_id) {
    auto interface_repr = get_string_repr(interface_params[arg_id]);
    auto derived_repr = get_string_repr(derived_params[arg_id]);

    kphp_error(interface_repr == derived_repr,
      format("default value of interface parameter:`%s` may not differ from value of derived parameter: `%s`, in function: %s",
             TermStringFormat::paint(interface_repr, TermStringFormat::green).c_str(),
             TermStringFormat::paint(derived_repr, TermStringFormat::green).c_str(),
             TermStringFormat::paint(derived_method->get_human_readable_name(), TermStringFormat::red).c_str()));
  }

  return true;
}

bool check_that_signatures_are_same(ClassPtr context_class, FunctionPtr interface_function) {
  auto interface_method_in_derived = context_class->members.get_instance_method(interface_function->local_name());
  return check_that_signatures_are_same(interface_function, context_class, interface_method_in_derived);
}

bool check_that_function_is_abstract(FunctionPtr interface_function) {
  auto interface_class = interface_function->class_id;

  return interface_class &&
         interface_class->is_interface() &&
         interface_function->root->cmd()->empty();
}

void check_static_function(FunctionPtr interface_function, std::vector<ClassPtr> &derived_classes) {
  for (auto derived : derived_classes) {
    auto static_in_derived = derived->members.get_static_method(interface_function->local_name());
    check_that_signatures_are_same(interface_function, derived, static_in_derived);
  }
}

} // namespace


/**
 * generated AST for interface function will be like this:
 *
 * function interface_function($param1, ...) {
 *   if ($this instanceof Derived_first) {
 *     return $this->interface_function($param1, ...);
 *   } else if ($this instanceof Derived_second) {
 *     return $this->interface_function($param1, ...);
 *   } else {
 *     return $this->interface_function($param1, ...);
 *   }
 */
void generate_body_of_interface_method(FunctionPtr interface_function) {
  auto &derived_classes = interface_function->class_id->derived_classes;

  if (derived_classes.empty()) {
    return;
  }

  kphp_assert(check_that_function_is_abstract(interface_function));
  if (interface_function->is_static_function()) {
    return check_static_function(interface_function, derived_classes);
  }

  VertexAdaptor<op_seq> body_of_interface_method;
  for (auto derived : derived_classes) {
    auto is_instance_of_derived = GenTree::conv_to<tp_bool>(create_instanceof(ClassData::gen_vertex_this({}), derived));

    VertexAdaptor<op_seq> call_derived_method;
    if (check_that_signatures_are_same(derived, interface_function)) {
      auto generated_this = ClassData::gen_vertex_this({});
      VertexPtr this_var = generated_this;
      if (derived_classes.size() == 1) {
        this_var = create_instance_cast_to(generated_this, derived);
      }
      call_derived_method = VertexAdaptor<op_seq>::create(VertexAdaptor<op_return>::create(GenTree::generate_call_on_instance_var(this_var, interface_function)));
    }

    if (body_of_interface_method) {
      body_of_interface_method = VertexAdaptor<op_seq>::create(VertexAdaptor<op_if>::create(is_instance_of_derived, call_derived_method, body_of_interface_method));
    } else {
      body_of_interface_method = call_derived_method;
    }
  }

  auto &root = interface_function->root;
  auto declaration_location = get_location(root);
  root = VertexAdaptor<op_function>::create(root->params(), body_of_interface_method);
  root->func_id = interface_function;
  interface_function->type = FunctionData::func_local;

  std::function<void(VertexPtr)> update_location = [&](VertexPtr v) {
    set_location(v, declaration_location);
    std::for_each(v->begin(), v->end(), update_location);
  };
  update_location(root);
}
