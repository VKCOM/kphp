#include "compiler/data/virtual-method-generator.h"

#include <queue>

#include "common/termformat/termformat.h"

#include "compiler/const-manipulations.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/debug.h"
#include "compiler/gentree.h"
#include "compiler/stage.h"

namespace {

VertexAdaptor<op_func_call> create_call_with_var_and_class_name_params(VertexAdaptor<op_var> instance_var, ClassPtr klass) {
  auto class_name = VertexAdaptor<op_func_name>::create();
  class_name->set_string("\\" + klass->name + "::class");

  return VertexAdaptor<op_func_call>::create(instance_var, class_name);
}

VertexAdaptor<op_func_call> create_instance_cast_to(VertexAdaptor<op_var> instance_var, ClassPtr derived) {
  auto cast_to_derived = create_call_with_var_and_class_name_params(instance_var, derived);
  cast_to_derived->set_string("instance_cast");
  return cast_to_derived;
}

template<class ClassMemberMethod>
bool check_that_signatures_are_same(FunctionPtr interface_function, ClassPtr context_class, ClassMemberMethod *interface_method_in_derived) {
  stage::set_line(get_location(interface_function->root).line);
  if (!interface_method_in_derived) {
    kphp_error(context_class->modifiers.is_abstract(),
               fmt_format("class: {} must be abstract, method: {} is not overridden",
                          TermStringFormat::paint_green(context_class->name),
                          TermStringFormat::paint_green(interface_function->get_human_readable_name())));
    return true;
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
    kphp_error(false, fmt_format("Count of arguments are different in interface method: `{}` and in class: `{}`",
                                 interface_function->get_human_readable_name(),
                                 context_class->name));

    return false;
  }

  auto get_string_repr = [] (VertexPtr v) {
    auto param = v.try_as<op_func_param>();
    kphp_assert(param);
    if (!param->has_default_value()) {
      return std::string{"NoDefault"};
    }
    return VertexPtrFormatter::to_string(param->default_value());
  };

  for (auto arg_id = interface_function->get_min_argn(); arg_id < i_argn; ++arg_id) {
    auto interface_repr = get_string_repr(interface_params[arg_id]);
    auto derived_repr = get_string_repr(derived_params[arg_id]);

    kphp_error(interface_repr == derived_repr,
      fmt_format("default value of interface parameter:`{}` may not differ from value of derived parameter: `{}`, in function: {}",
                 TermStringFormat::paint(interface_repr, TermStringFormat::green),
                 TermStringFormat::paint(derived_repr, TermStringFormat::green),
                 TermStringFormat::paint(derived_method->get_human_readable_name(), TermStringFormat::red)));
  }

  return true;
}

bool check_that_signatures_are_same(ClassPtr context_class, FunctionPtr interface_function) {
  auto interface_method_in_derived = context_class->members.get_instance_method(interface_function->local_name());
  return check_that_signatures_are_same(interface_function, context_class, interface_method_in_derived);
}

void check_static_function(FunctionPtr interface_function, std::vector<ClassPtr> &derived_classes) {
  for (auto derived : derived_classes) {
    auto static_in_derived = derived->members.get_static_method(interface_function->local_name());
    check_that_signatures_are_same(interface_function, derived, static_in_derived);
  }
}

VertexAdaptor<op_case> gen_case_on_hash(ClassPtr derived, VertexAdaptor<op_seq> cmd) {
  auto hash_of_derived = VertexAdaptor<op_int_const>::create();
  hash_of_derived->str_val = std::to_string(derived->get_hash());

  return VertexAdaptor<op_case>::create(hash_of_derived, cmd);
}

VertexAdaptor<op_case> gen_case_calling_methods_on_derived_class(ClassPtr derived, FunctionPtr virtual_function) {
  FunctionPtr concrete_method_of_derived;
  if (auto method_of_derived = derived->members.get_instance_method(virtual_function->local_name())) {
    concrete_method_of_derived = method_of_derived->function;
  } else {
    if (auto method_from_ancestor = derived->get_instance_method(virtual_function->local_name())) {
      concrete_method_of_derived = method_from_ancestor->function;
    }

    bool is_overridden_by_ancestors = concrete_method_of_derived && !concrete_method_of_derived->modifiers.is_abstract();
    if (virtual_function->modifiers.is_abstract() && !derived->modifiers.is_abstract() && !is_overridden_by_ancestors) {
      kphp_error(false, fmt_format("You should override abstract method: `{}` in class: `{}`",
                                   virtual_function->get_human_readable_name(),
                                   derived->name));
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

  VertexPtr this_var = create_instance_cast_to(ClassData::gen_vertex_this({}), derived);
  // generate concrete_method call, with arguments from virtual_functions, because of Derived can have extra default params:
  auto call_self_method_of_derived = GenTree::generate_call_on_instance_var(this_var, virtual_function, concrete_method_of_derived->local_name());
  auto call_derived_method = VertexAdaptor<op_seq>::create(VertexAdaptor<op_return>::create(call_self_method_of_derived));

  return gen_case_on_hash(derived, call_derived_method);
}

} // namespace


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
 *   case 0x02280228: { // hash of Derived 2
 *     $tmp = instance_cast<Derived2>($this);
 *     $tmp->virtual_function($param1, ...);
 *     break;
 *   }
 *   default: {
 *     php_warning("call method(Interface::virtual_function) on empty class
 *     exit(0);
 *   }
 */
void generate_body_of_virtual_method(FunctionPtr virtual_function) {
  auto klass = virtual_function->class_id;
  kphp_assert(klass);

  auto &derived_classes = klass->derived_classes;

  if (derived_classes.empty()) {
    return;
  }

  if (virtual_function->modifiers.is_static()) {
    kphp_assert(klass->modifiers.is_abstract());
    return check_static_function(virtual_function, derived_classes);
  }

  if (!virtual_function->modifiers.is_abstract()) {
    kphp_assert(klass->members.has_instance_method(virtual_function->get_name_of_self_method()));
    kphp_assert(virtual_function->root->cmd()->empty());
  }

  std::vector<VertexPtr> cases;
  for (auto inheritor : klass->get_all_inheritors()) {
    if (auto case_for_cur_class = gen_case_calling_methods_on_derived_class(inheritor, virtual_function)) {
      cases.emplace_back(case_for_cur_class);
    }
  }

  if (!cases.empty()) {
    auto message_that_class_is_empty = VertexAdaptor<op_string>::create();
    message_that_class_is_empty->str_val = fmt_format("call method({}) on null object", virtual_function->get_human_readable_name());

    auto warn_on_default = VertexAdaptor<op_func_call>::create(message_that_class_is_empty);
    warn_on_default->str_val = "critical_error";

    auto call_of_exit = VertexAdaptor<op_seq>::create(warn_on_default);
    cases.emplace_back(VertexAdaptor<op_default>::create(call_of_exit));
  } else {
    // just keep empty body, when there is no inheritors for interface method
    return;
  }

  auto get_hash_of_this = VertexAdaptor<op_func_call>::create(ClassData::gen_vertex_this({}));
  get_hash_of_this->set_string("get_hash_of_class");

  auto body_of_virtual_method = VertexAdaptor<op_seq>::create(GenTree::create_switch_vertex(virtual_function, get_hash_of_this, std::move(cases)));

  auto &root = virtual_function->root;
  auto declaration_location = get_location(root);
  root = VertexAdaptor<op_function>::create(root->params(), body_of_virtual_method);
  root->func_id = virtual_function;
  virtual_function->type = FunctionData::func_local;

  std::function<void(VertexPtr)> update_location = [&](VertexPtr v) {
    set_location(v, declaration_location);
    std::for_each(v->begin(), v->end(), update_location);
  };
  update_location(root);
}
