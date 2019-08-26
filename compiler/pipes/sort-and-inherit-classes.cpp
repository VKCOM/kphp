#include "compiler/pipes/sort-and-inherit-classes.h"

#include <unordered_map>

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/function-pass.h"
#include "compiler/gentree.h"
#include "compiler/phpdoc.h"
#include "compiler/stage.h"
#include "compiler/threading/profiler.h"
#include "compiler/utils/string-utils.h"
#include "common/algorithms/hashes.h"

/**
 * Через этот pass проходят функции вида
 * baseclassname$$fname$$contextclassname
 * которые получились клонированием родительского дерева
 * В будущем этот pass должен уйти, когда лямбды будут вычленяться не на уровне gentree, а позже
 */
class PatchInheritedMethodPass : public FunctionPassBase {
  DataStream<FunctionPtr> &function_stream;
public:
  explicit PatchInheritedMethodPass(DataStream<FunctionPtr> &function_stream) :
    function_stream(function_stream) {}

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *) {
    if (auto call = root.try_as<op_constructor_call>()) {
      if (call->func_id && call->func_id->is_lambda()) {
        ClassPtr lambda_class = call->func_id->class_id;
        FunctionPtr invoke_method = lambda_class->members.get_instance_method(ClassData::NAME_OF_INVOKE_METHOD)->function;
        vector<VertexAdaptor<op_func_param>> uses_of_lambda;

        lambda_class->members.for_each([&](ClassMemberInstanceField &f) {
          auto new_var_use = VertexAdaptor<op_var>::create();
          new_var_use->set_string(f.local_name());
          set_location(new_var_use, f.root->location);
          auto func_param = VertexAdaptor<op_func_param>::create(new_var_use);
          set_location(func_param, f.root->location);

          uses_of_lambda.insert(uses_of_lambda.begin(), func_param);
        });

        return GenTree::generate_anonymous_class(invoke_method->root, current_function, true, std::move(uses_of_lambda))
          .require(function_stream)
          .get_generated_lambda()
          ->gen_constructor_call_pass_fields_as_args();
      }
    }

    return root;
  }
};

TSHashTable<SortAndInheritClassesF::wait_list> SortAndInheritClassesF::ht;

// если все dependents класса уже обработаны, возвращает nullptr
// если же какой-то из dependents (класс/интерфейс) ещё не обработан (его надо подождать), возвращает указатель на его
auto SortAndInheritClassesF::get_not_ready_dependency(ClassPtr klass) -> decltype(ht)::HTNode* {
  for (const auto &dep : klass->str_dependents) {
    auto node = ht.at(vk::std_hash(dep.class_name));
    kphp_assert(node);
    if (!node->data.done) {
      return node;
    }
  }

  return {};
}

// делаем функцию childclassname$$localname, которая выглядит как
// function childclassname$$localname($args) { return baseclassname$$localname$$childclassname(...$args); }
VertexAdaptor<op_function> SortAndInheritClassesF::generate_function_with_parent_call(ClassPtr child_class, const ClassMemberStaticMethod &parent_method) {
  auto local_name = parent_method.local_name();
  auto parent_f = parent_method.function;

  auto parent_function_name = parent_f->name + "$$" + replace_backslashes(child_class->name);
  // it's equivalent to new_func_call->set_string("parent::" + local_name);

  VertexAdaptor<op_func_call> new_func_call;
  auto parent_params = parent_f->get_params_as_vector_of_vars();
  if (!parent_f->has_variadic_param) {
    new_func_call = VertexAdaptor<op_func_call>::create(parent_params);
  } else {
    kphp_assert(!parent_params.empty());
    auto &last_param = parent_params.back();
    auto unpacked_last_param = VertexAdaptor<op_varg>::create(last_param).set_location(last_param);
    parent_params.pop_back();

    new_func_call = VertexAdaptor<op_func_call>::create(parent_params, unpacked_last_param);
  }
  new_func_call->set_string(parent_function_name);

  auto new_return = VertexAdaptor<op_return>::create(new_func_call);
  auto new_cmd = VertexAdaptor<op_seq>::create(new_return);
  auto new_params = parent_f->root->params().clone();
  auto func = VertexAdaptor<op_function>::create(new_params, new_cmd);
  func->copy_location_and_flags(*parent_f->root);

  return func;
}


// клонировать функцию baseclassname$$fname в контекстную baseclassname$$fname$$contextclassname
// (контекстные нужны для реализации статического наследования)
FunctionPtr SortAndInheritClassesF::create_function_with_context(FunctionPtr parent_f, const std::string &ctx_function_name) {
  auto root = parent_f->root.clone();
  auto context_function = FunctionData::clone_from(ctx_function_name, parent_f, root);

  return context_function;
}

void SortAndInheritClassesF::inherit_static_method_from_parent(ClassPtr child_class, const ClassMemberStaticMethod &parent_method, DataStream<FunctionPtr> &function_stream) {
  auto local_name = parent_method.local_name();
  auto parent_f = parent_method.function;
  auto parent_class = parent_f->class_id;
  /**
   * A::f() -> B -> C; при A->B сделался A::f$$B
   * но при B->C должно быть A::f$$C, а не B::f$$C
   * (чтобы B::f$$C не считать required)
   */
  if (parent_f->is_auto_inherited || parent_f->context_class != parent_class) {
    return;
  }

  if (auto child_method = child_class->members.get_static_method(local_name)) {
    kphp_error_return(!parent_f->is_final,
                      format("Can not override method marked as 'final': %s", parent_f->get_human_readable_name().c_str()));
    kphp_error_return(parent_method.access_type != access_static_private,
                      format("Can not override private method: %s", parent_f->get_human_readable_name().c_str()));
    kphp_error_return(parent_method.access_type == child_method->access_type,
                      format("Can not change access type for method: %s", child_method->function->get_human_readable_name().c_str()));
  } else {
    auto child_root = generate_function_with_parent_call(child_class, parent_method);

    string new_name = replace_backslashes(child_class->name) + "$$" + parent_method.local_name();
    FunctionPtr child_function = FunctionData::clone_from(new_name, parent_f, child_root);
    child_function->is_auto_inherited = true;
    child_function->is_inline = true;

    child_class->members.add_static_method(child_function, parent_f->access_type);    // пока наследование только статическое

    G->register_and_require_function(child_function, function_stream);
  }

  // контекстная функция имеет название baseclassname$$fname$$contextclassname
  // это склонированное дерево baseclassname$$fname (parent_f) + патч для лямбд (который должен когда-то уйти)
  string ctx_function_name = replace_backslashes(parent_class->name) + "$$" + local_name + "$$" + replace_backslashes(child_class->name);
  FunctionPtr context_function = create_function_with_context(parent_f, ctx_function_name);
  context_function->context_class = child_class;

  PatchInheritedMethodPass pass(function_stream);
  run_function_pass(context_function, &pass);
  stage::set_file(child_class->file_id);
  stage::set_function(FunctionPtr{});

  G->register_and_require_function(context_function, function_stream);
}



void SortAndInheritClassesF::inherit_child_class_from_parent(ClassPtr child_class, ClassPtr parent_class, DataStream<FunctionPtr> &function_stream) {
  stage::set_file(child_class->file_id);
  stage::set_function(FunctionPtr{});
  
  kphp_error_return(parent_class->is_class() && child_class->is_class(),
                    format("Error extends %s and %s", child_class->name.c_str(), parent_class->name.c_str()));

  child_class->parent_class = parent_class;
  child_class->check_parent_constructor();

  // A::f -> B -> C -> D; для D нужно C::f$$D, B::f$$D, A::f$$D
  for (; parent_class; parent_class = parent_class->parent_class) {
    parent_class->members.for_each([&](const ClassMemberStaticMethod &m) {
      inherit_static_method_from_parent(child_class, m, function_stream);
    });
    parent_class->members.for_each([&](const ClassMemberStaticField &f) {
      if (auto field = child_class->members.get_static_field(f.local_name())) {
        kphp_error(f.access_type != access_static_private,
                   format("Can't redeclare private static field %s in class %s\n", f.local_name().c_str(), child_class->name.c_str()));
        kphp_error(f.access_type == field->access_type,
                   format("Can't change access type for static field %s in class %s\n", f.local_name().c_str(), child_class->name.c_str()));
      }
    });
  }

  if (child_class->parent_class) {
    AutoLocker<Lockable *> locker(&(*child_class->parent_class));
    child_class->parent_class->derived_classes.emplace_back(child_class);
  }
}

void SortAndInheritClassesF::inherit_class_from_interface(ClassPtr child_class, InterfacePtr interface_class, DataStream<FunctionPtr> &function_stream) {
  kphp_error(interface_class->is_interface(),
             format("Error implements %s and %s", child_class->name.c_str(), interface_class->name.c_str()));
  if (child_class->is_interface()) {
    child_class->parent_class = interface_class;

    auto clone_function = [child_class, &function_stream](FunctionPtr from) {
      auto new_root = from->root.clone();
      auto cloned_fun = FunctionData::clone_from(child_class->name + "$$" + get_local_name_from_global_$$(from->name), from, new_root);
      cloned_fun->class_id = child_class;
      G->register_and_require_function(cloned_fun, function_stream, true);

      return cloned_fun;
    };

    interface_class->members.for_each([&](const ClassMemberInstanceMethod &m) {
      child_class->members.add_instance_method(clone_function(m.function), m.access_type);
    });

    interface_class->members.for_each([&](const ClassMemberStaticMethod &m) {
      child_class->members.add_static_method(clone_function(m.function), m.access_type);
    });
  } else {
    child_class->implements.emplace_back(interface_class);
  }

  AutoLocker<Lockable *> locker(&(*interface_class));
  interface_class->derived_classes.emplace_back(child_class);
}

namespace {
using name_to_classes_methods_t = std::unordered_map<std::string, std::vector<FunctionPtr>>;

name_to_classes_methods_t collect_and_mark_methods_with_the_same_name(ClassPtr cur_klass) {
  name_to_classes_methods_t name_to_methods;

  for (auto derived : cur_klass->derived_classes) {
    auto derived_methods = collect_and_mark_methods_with_the_same_name(derived);
    for (auto &name_methods : derived_methods) {
      auto &methods_with_the_same_name_in_derived = name_to_methods[name_methods.first];
      methods_with_the_same_name_in_derived.insert(methods_with_the_same_name_in_derived.end(), name_methods.second.begin(), name_methods.second.end());
    }
  }

  cur_klass->members.for_each([&](ClassMemberInstanceMethod &method) {
    if (method.function->is_constructor() || method.function->access_type == AccessType::access_private) {
      return;
    }

    auto &methods_with_the_same_name_in_derived = name_to_methods[method.local_name()];

    if (!methods_with_the_same_name_in_derived.empty()) {
      method.function->is_virtual_method = true;
      method.function->is_overridden_method = false;
    }

    for (auto &derived_m : methods_with_the_same_name_in_derived) {
      derived_m->is_overridden_method = true;
    }

    methods_with_the_same_name_in_derived.push_back(method.function);
  });

  return name_to_methods;
}

/**
 * Imagine situation:
 * class B { public function foo() {} }
 *
 * class D extends B {}
 *
 * class DD extends D { public function foo() {} }
 * class DD2 extends D { public function foo() {} }
 *
 * in class D we need generate virtual method `foo` for choosing appropriate derived class
 */
void generate_empty_virtual_method_in_parents(FunctionPtr method, DataStream<FunctionPtr> &os) {
  auto method_klass = method->class_id;
  for (auto parent = method_klass->parent_class; parent; parent = parent->parent_class) {
    if (parent->is_interface() || parent->members.get_instance_method(method->local_name())) {
      return;
    }

    auto new_name = parent->name + "$$" + method->local_name();

    auto empty_parent_method_vertex = VertexAdaptor<op_function>::create(method->root->params().clone(), VertexAdaptor<op_seq>::create());
    auto empty_parent_method = FunctionData::clone_from(new_name, method, empty_parent_method_vertex);
    parent->members.add_instance_method(empty_parent_method, empty_parent_method->access_type);

    empty_parent_method->is_virtual_method = true;
    empty_parent_method->is_overridden_method = true;

    G->register_and_require_function(empty_parent_method, os, true);
  }
}

void mark_virtual_and_overridden_methods(ClassPtr cur_klass, DataStream<FunctionPtr> &os) {
  name_to_classes_methods_t methods = collect_and_mark_methods_with_the_same_name(cur_klass);

  for (auto &name_and_methods : methods) {
    auto &methods_with_the_same_name = name_and_methods.second;
    for (auto m : methods_with_the_same_name) {
      if (m->is_overridden_method) {
        generate_empty_virtual_method_in_parents(m, os);
      }
      if (m->is_virtual_method && !m->root->cmd()->empty()) {
        m->move_virtual_to_self_method();
      }
    }
  }
}

template<class ClassMemberT>
class CheckParentDoesntHaveMemberWithSameName {
private:
  ClassPtr c;

public:
  explicit CheckParentDoesntHaveMemberWithSameName(ClassPtr cur_class) :
    c(cur_class) {
  }

  void operator()(const ClassMemberT &member) const {
    auto field_name = member.local_name();
    auto par = c->parent_class;
    bool parent_has_instance_or_static_field = par->get_instance_field(field_name) ||
                                               (!std::is_same<ClassMemberT, ClassMemberConstant>{} && par->get_constant(field_name)) ||
                                               (!std::is_same<ClassMemberT, ClassMemberStaticField>{} && par->get_static_field(field_name));
    kphp_error(!parent_has_instance_or_static_field,
               format("You may not override field: `%s`, in class: `%s`", field_name.c_str(), c->name.c_str()));
  }
};
} // namespace

/**
 * Каждый класс поступает сюда один и ровно один раз — когда он и все его dependents
 * (родители, трейты, интерфейсы) тоже готовы.
 */
void SortAndInheritClassesF::on_class_ready(ClassPtr klass, DataStream<FunctionPtr> &function_stream) {
  for (const auto &dep : klass->str_dependents) {
    ClassPtr dep_class = G->get_class(dep.class_name);

    switch (dep.type) {
      case ClassType::klass:
        inherit_child_class_from_parent(klass, dep_class, function_stream);
        break;
      case ClassType::interface:
        inherit_class_from_interface(klass, dep_class, function_stream);
        break;
      case ClassType::trait:
        kphp_assert(0 && "mixin traits is not supported yet");
        break;
    }
  }

  if (!klass->phpdoc_str.empty()) {
    analyze_class_phpdoc(klass);
  }
}

inline void SortAndInheritClassesF::analyze_class_phpdoc(ClassPtr klass) {
  if (PhpDocTypeRuleParser::is_tag_in_phpdoc(klass->phpdoc_str, php_doc_tag::kphp_memcache_class)) {
      G->set_memcache_class(klass);
  }
}

void SortAndInheritClassesF::execute(ClassPtr klass, MultipleDataStreams<FunctionPtr, ClassPtr> &os) {
  auto &function_stream = *os.template project_to_nth_data_stream<0>();
  auto &restart_class_stream = *os.template project_to_nth_data_stream<1>();

  if (auto dependency = get_not_ready_dependency(klass)) {
    AutoLocker<Lockable*> locker(dependency);
    if (dependency->data.done) {              // вдруг между вызовом ready и этим моментом стало done
      restart_class_stream << klass;
    } else {
      dependency->data.waiting.emplace_front(klass);
    }
    return;
  }

  on_class_ready(klass, function_stream);

  auto node = ht.at(vk::std_hash(klass->name));
  kphp_assert(!node->data.done);

  AutoLocker<Lockable *> locker(node);
  for (ClassPtr restart_klass: node->data.waiting) {    // все классы, которые ждали текущего —
    restart_class_stream << restart_klass;              // запустить SortAndInheritClassesF ещё раз для них
  }

  node->data.waiting.clear();
  node->data.done = true;
}

void SortAndInheritClassesF::check_on_finish(DataStream<FunctionPtr> &os) {
  DataStream<FunctionPtr> generated_empty_methods_in_parent{true};
  std::vector<ClassPtr> polymorphic_classes;
  for (auto c : G->get_classes()) {
    auto node = ht.at(vk::std_hash(c->name));
    kphp_error(node->data.done, format("class `%s` has unresolved dependencies", c->name.c_str()));
    kphp_error_return(c->implements.empty() || !c->parent_class,
      format("You may not `extends` and `implements` simultaneously, class: %s", c->name.c_str()));

    bool is_top_of_hierarchy = !c->parent_class && !c->derived_classes.empty();
    if (is_top_of_hierarchy) {
      mark_virtual_and_overridden_methods(c, generated_empty_methods_in_parent);
    }

    if (c->parent_class) {
      c->members.for_each(CheckParentDoesntHaveMemberWithSameName<ClassMemberInstanceField>{c});
      c->members.for_each(CheckParentDoesntHaveMemberWithSameName<ClassMemberStaticField>{c});
      c->members.for_each(CheckParentDoesntHaveMemberWithSameName<ClassMemberConstant>{c});

      c->members.for_each([&c](const ClassMemberStaticMethod &m) {
        kphp_error(!c->parent_class->get_instance_method(m.local_name()),
                   format("Cannot make non static method `%s` static", m.function->get_human_readable_name().c_str()));
      });
    }

    if (!c->is_builtin() && c->is_polymorphic_class()) {
      polymorphic_classes.emplace_back(c);
    }

    // For stable code generation of interface method body
    if (c->is_interface()) {
      std::sort(c->derived_classes.begin(), c->derived_classes.end());
    }
  }

  stage::die_if_global_errors();
  for (auto c : polymorphic_classes) {
      auto virt_clone = c->add_virt_clone();
      G->register_and_require_function(virt_clone, generated_empty_methods_in_parent, true);
  }

  for (auto fun : generated_empty_methods_in_parent.flush()) {
    os << fun;
  }
}
