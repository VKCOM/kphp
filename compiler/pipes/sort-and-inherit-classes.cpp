// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/sort-and-inherit-classes.h"

#include "common/algorithms/compare.h"
#include "common/algorithms/contains.h"
#include "common/algorithms/hashes.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/name-gen.h"
#include "compiler/stage.h"
#include "compiler/threading/profiler.h"
#include "compiler/pipes/clone-nested-lambdas.h"
#include "compiler/utils/string-utils.h"


TSHashTable<SortAndInheritClassesF::wait_list> SortAndInheritClassesF::ht;

namespace {
void mark_virtual_and_overridden_methods(ClassPtr cur_class, DataStream<FunctionPtr> &os) {
  static std::unordered_set<ClassPtr> visited_classes;
  if (vk::contains(visited_classes, cur_class)) {
    return;
  }
  visited_classes.insert(cur_class);

  auto find_virtual_overridden = [&](ClassMemberInstanceMethod &method) {
    if (!method.function || method.function->is_constructor() || method.function->modifiers.is_private()) {
      return;
    }

    for (const auto &ancestor : cur_class->get_all_ancestors()) {
      if (ancestor == cur_class) {
        continue;
      }
      if (auto *parent_member = ancestor->members.get_instance_method(method.local_name())) {
        auto parent_function = parent_member->function;

        kphp_error(parent_function->modifiers.is_abstract() || !parent_function->modifiers.is_final(),
                   fmt_format("You cannot override final method: {}", method.function->as_human_readable()));

        kphp_error(parent_function->modifiers.access_modifier() == method.function->modifiers.access_modifier(),
                   fmt_format("Can not change access type for method: {}", method.function->as_human_readable()));

        method.function->is_overridden_method = true;

        if (!parent_function->root->cmd()->empty()) {
          parent_function->move_virtual_to_self_method(os);
          parent_function->is_virtual_method = true;
        }

        return;
      }
    }
  };

  cur_class->members.for_each(find_virtual_overridden);
  for (auto derived : cur_class->derived_classes) {
    mark_virtual_and_overridden_methods(derived, os);
  }
}

bool clone_method(FunctionPtr from, ClassPtr to_class, DataStream<FunctionPtr> &function_stream) {
  if (from->modifiers.is_instance() && to_class->members.has_instance_method(from->local_name())) {
    return false;
  } else if (from->modifiers.is_static() && to_class->members.has_static_method(from->local_name())) {
    return false;
  }

  std::string cloned_fname = replace_backslashes(to_class->name) + "$$" + from->local_name();
  FunctionPtr cloned_fun = FunctionData::clone_from(from, cloned_fname);
  CloneNestedLambdasPass::run_if_lambdas_inside(cloned_fun, &function_stream);
  if (from->modifiers.is_instance()) {
    to_class->members.add_instance_method(cloned_fun);
  } else if (from->modifiers.is_static()) {
    to_class->members.add_static_method(cloned_fun);
  }

  G->register_and_require_function(cloned_fun, function_stream, true);
  return true;
};

} // namespace


// Returns nullptr if all class dependents are already processed.
// Otherwise it returns a pointer to a dependent class/interface that needs to be processed.
auto SortAndInheritClassesF::get_not_ready_dependency(ClassPtr klass) -> decltype(ht)::HTNode* {
  for (const auto &dep : klass->get_str_dependents()) {
    auto *node = ht.at(vk::std_hash(dep.class_name));
    kphp_assert(node);
    if (!node->data.done) {
      return node;
    }
  }

  return {};
}

// Generate a childclassname$$localname function that looks like this:
// function childclassname$$localname($args) { return baseclassname$$localname$$childclassname(...$args); }
VertexAdaptor<op_function> SortAndInheritClassesF::generate_function_with_parent_call(ClassPtr child_class, const ClassMemberStaticMethod &parent_method) {
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
  auto new_param_list = parent_f->root->param_list().clone();
  auto func = VertexAdaptor<op_function>::create(new_param_list, new_cmd).set_location(parent_f->root);

  return func;
}

// PHP has late static binding, that's why inheriting a static method is not trivial
// a new function is created with context_class (to resolve 'static')
void SortAndInheritClassesF::inherit_static_method_from_parent(ClassPtr child_class, const ClassMemberStaticMethod &parent_method, DataStream<FunctionPtr> &function_stream) {
  auto local_name = parent_method.local_name();
  auto parent_f = parent_method.function;
  auto parent_class = parent_f->class_id;
   // A::f() -> B -> C; with A->B becomes A::f$$B
   // but with B->C it should be A::f$$C and not B::f$$C
   // so we don't assume B::f$$C as required
  if (parent_f->is_auto_inherited || parent_f->context_class != parent_class) {
    return;
  }

  if (const auto *child_method = child_class->members.get_static_method(local_name)) {
    kphp_error(!parent_f->modifiers.is_final(),
               fmt_format("Can not override method marked as 'final': {}", parent_f->as_human_readable()));
    kphp_error(!(parent_method.function->modifiers.is_static() && parent_method.function->modifiers.is_private()),
               fmt_format("Can not override private method: {}", parent_f->as_human_readable()));
    kphp_error(parent_method.function->modifiers.access_modifier() == child_method->function->modifiers.access_modifier(),
               fmt_format("Can not change access type for method: {}", child_method->function->as_human_readable()));
    kphp_error(!(!parent_f->modifiers.is_abstract() && child_method->function->modifiers.is_abstract()),
               fmt_format("Can not make non-abstract method {} abstract in class {}", parent_f->as_human_readable(), child_class->name));
  } else {
    auto child_root = generate_function_with_parent_call(child_class, parent_method);

    std::string new_name = replace_backslashes(child_class->name) + "$$" + parent_method.local_name();
    FunctionPtr child_function = FunctionData::clone_from(parent_f, new_name, child_root);
    child_function->is_auto_inherited = true;
    child_function->is_inline = true;

    child_class->members.add_static_method(child_function);
    G->register_and_require_function(child_function, function_stream);
  }

  // contextual function has a name like baseclassname$$fname$$contextclassname;
  // it's a cloned tree of baseclassname$$fname (parent_f) + kludge for lambdas
  std::string ctx_function_name = replace_backslashes(parent_class->name) + "$$" + local_name + "$$" + replace_backslashes(child_class->name);
  FunctionPtr context_function = FunctionData::clone_from(parent_f, ctx_function_name);
  context_function->context_class = child_class;
  context_function->outer_function = parent_f;
  CloneNestedLambdasPass::run_if_lambdas_inside(context_function, &function_stream);
  stage::set_file(child_class->file_id);
  stage::set_function(FunctionPtr{});

  G->register_and_require_function(context_function, function_stream);
}

// when Base::f and Child::f (non-static) both exist, we just do some checks
// (Base could be a class or an interface)
void SortAndInheritClassesF::inherit_instance_method_from_parent(ClassPtr child_class, const ClassMemberInstanceMethod &parent_method) {
  const auto *child_method = child_class->members.get_instance_method(parent_method.local_name());
  if (!child_method) {
    return;
  }

  FunctionPtr parent_f = parent_method.function;
  FunctionPtr child_f = child_method->function;
  stage::set_location(Location{child_class->file_id, child_f, child_f->root->location.line});

  kphp_error(!(!parent_f->modifiers.is_abstract() && child_f->modifiers.is_abstract()),
             fmt_format("Can not make non-abstract method {} abstract in class {}", parent_f->as_human_readable(), child_class->name));
  kphp_error(!(parent_f->is_generic() && child_f->is_generic() && !parent_f->modifiers.is_abstract()),
             fmt_format("Overriding non-abstract generic methods is forbidden: {}", child_f->as_human_readable()));
  kphp_error(!(parent_f->is_generic() && !child_f->is_generic()),
             fmt_format("{} is a generic method, but child {} is not.\n@kphp-generic is not inherited, add it manually to child methods", parent_f->as_human_readable(), child_f->as_human_readable()));
  kphp_error(!(child_f->is_generic() && !parent_f->is_generic()),
             fmt_format("{} is a generic method, but parent {} is not", child_f->as_human_readable(), parent_f->as_human_readable()));
}


void SortAndInheritClassesF::inherit_child_class_from_parent(ClassPtr child_class, ClassPtr parent_class, DataStream<FunctionPtr> &function_stream) {
  stage::set_file(child_class->file_id);
  stage::set_function(FunctionPtr{});

  if (parent_class->is_ffi_cdata() || parent_class->name == "FFI\\Scope") {
    kphp_error_return(child_class->is_builtin() || child_class->is_ffi_scope(),
                      "FFI classes should not be extended");
  } else {
    kphp_error_return(parent_class->is_class() && child_class->is_class(),
                      fmt_format("Error extends {} and {}", child_class->name, parent_class->name));

  }

  kphp_error_return(!parent_class->modifiers.is_final(),
                    fmt_format("You cannot extends final class: {}", child_class->name));

  child_class->parent_class = parent_class;

  // A::f -> B -> C -> D; for D we require C::f$$D, B::f$$D, A::f$$D
  for (; parent_class; parent_class = parent_class->parent_class) {
    parent_class->members.for_each([&](const ClassMemberStaticMethod &m) {
      inherit_static_method_from_parent(child_class, m, function_stream);
    });
    parent_class->members.for_each([&](const ClassMemberInstanceMethod &m) {
      inherit_instance_method_from_parent(child_class, m);
    });
    parent_class->members.for_each([&](const ClassMemberStaticField &f) {
      if (const auto *field = child_class->members.get_static_field(f.local_name())) {
        kphp_error(!f.modifiers.is_private(),
                   fmt_format("Can't redeclare private static field {} in class {}\n", f.local_name(), child_class->name));

        kphp_error(f.modifiers == field->modifiers,
                   fmt_format("Can't change access type for static field {} ({}) in class {} ({})\n",
                              f.local_name(), f.modifiers.to_string(),
                              child_class->name, field->modifiers.to_string())
        );
      }
    });
    parent_class->members.for_each([&](const ClassMemberConstant &c) {
      if (const auto *child_const = child_class->get_constant(c.local_name())) {
        kphp_error(child_const->access == c.access,
                   fmt_format("Can't change access type for constant {} in class {}", c.local_name(), child_class->name));
      }
    });
  }

  if (child_class->parent_class) {
    AutoLocker<Lockable *> locker(&(*child_class->parent_class));
    child_class->parent_class->derived_classes.emplace_back(child_class);
  }
}

void SortAndInheritClassesF::inherit_class_from_interface(ClassPtr child_class, InterfacePtr interface_class) {
  kphp_error(interface_class->is_interface(),
             fmt_format("{} `implements` a non-interface {}", child_class->name, interface_class->name));

  child_class->implements.emplace_back(interface_class);

  interface_class->members.for_each([&](const ClassMemberInstanceMethod &m) {
    inherit_instance_method_from_parent(child_class, m);
  });

  AutoLocker<Lockable *> locker(&(*interface_class));
  interface_class->derived_classes.emplace_back(child_class);
}

void SortAndInheritClassesF::clone_members_from_traits(std::vector<TraitPtr> &&traits, ClassPtr ready_class, DataStream<FunctionPtr> &function_stream) {
  for (size_t i = 0; i < traits.size(); ++i) {
    auto check_other_traits_doesnt_contain_method_and_clone = [&](FunctionPtr method) {
      if (!clone_method(method, ready_class, function_stream)) {
        return;
      }

      for (size_t j = i + 1; j < traits.size(); ++j) {
        if (traits[j]->members.has_instance_method(method->local_name()) || traits[j]->members.has_static_method(method->local_name())) {
          kphp_error(false, fmt_format("in class: {}, you have methods collision: {}", ready_class->as_human_readable(), method->as_human_readable()));
        }
      }
    };

    traits[i]->members.for_each([&](ClassMemberInstanceMethod &m) { check_other_traits_doesnt_contain_method_and_clone(m.function); });
    traits[i]->members.for_each([&](ClassMemberStaticMethod   &m) { check_other_traits_doesnt_contain_method_and_clone(m.function); });

    traits[i]->members.for_each([&](const ClassMemberInstanceField &f) {
      ready_class->members.add_instance_field(f.root.clone(), f.var->init_val.clone(), f.modifiers, f.phpdoc, f.type_hint);
    });

    traits[i]->members.for_each([&](const ClassMemberStaticField &f) {
      ready_class->members.add_static_field(f.root.clone(), f.var->init_val.clone(), f.modifiers, f.phpdoc, f.type_hint);
    });
  }

  if (ready_class->is_class()) {
    ready_class->members.for_each([&](ClassMemberInstanceMethod &m) {
      if (m.function->modifiers.is_abstract()) {
        kphp_error(ready_class->modifiers.is_abstract(),
                   fmt_format("class {} must be declared abstract, as it has an abstract method {}", ready_class->as_human_readable(), m.local_name()));
      }
    });
  }
}

// Every class is passed here exactly once - when that class and all of its dependents are ready.
void SortAndInheritClassesF::on_class_ready(ClassPtr klass, DataStream<FunctionPtr> &function_stream) {
  stage::set_file(klass->file_id);
  std::vector<TraitPtr> traits;

  // we have to resolve traits first to see overridden methods before static inheritance
  for (const auto &dep : klass->get_str_dependents()) {
    if (dep.type == ClassType::trait) {
      traits.emplace_back(G->get_class(dep.class_name));
    }
  }
  clone_members_from_traits(std::move(traits), klass, function_stream);

  for (const auto &dep : klass->get_str_dependents()) {
    ClassPtr dep_class = G->get_class(dep.class_name);

    switch (dep.type) {
      case ClassType::klass:
        inherit_child_class_from_parent(klass, dep_class, function_stream);
        break;
      case ClassType::interface:
        inherit_class_from_interface(klass, dep_class);
        break;
      case ClassType::ffi_cdata:
      case ClassType::ffi_scope:
        kphp_assert(vk::any_of_equal(dep_class->name, "FFI\\CData", "FFI\\Scope"));
        klass->parent_class = dep_class;
        break;
      case ClassType::trait:
        break;
    }
  }

  klass->create_default_constructor_if_required(function_stream);
}

void SortAndInheritClassesF::execute(ClassPtr klass, MultipleDataStreams<FunctionPtr, ClassPtr> &os) {
  auto &function_stream = *os.template project_to_nth_data_stream<0>();
  auto &restart_class_stream = *os.template project_to_nth_data_stream<1>();

  if (auto *dependency = get_not_ready_dependency(klass)) {
    AutoLocker<Lockable*> locker(dependency);
    // check whether done is changed at this point
    if (dependency->data.done) {
      restart_class_stream << klass;
    } else {
      dependency->data.waiting.emplace_front(klass);
    }
    return;
  }

  on_class_ready(klass, function_stream);

  auto *node = ht.at(vk::std_hash(klass->name));
  kphp_assert(!node->data.done);

  AutoLocker<Lockable *> locker(node);
  // for every class that was waiting for the current class
  // we run SortAndInheritClassesF again
  for (ClassPtr restart_klass: node->data.waiting) {
    restart_class_stream << restart_klass;
  }

  node->data.waiting.clear();
  node->data.done = true;
}

void SortAndInheritClassesF::check_on_finish(DataStream<FunctionPtr> &os) {
  DataStream<FunctionPtr> generated_self_methods{true};
  auto classes = G->get_classes();

  for (ClassPtr c : classes) {
    auto *node = ht.at(vk::std_hash(c->name));
    if (!node->data.done) {
      auto is_not_done = [](const ClassData::StrDependence &dep) { return !ht.at(vk::std_hash(dep.class_name))->data.done; };
      auto str_dep_to_string = [](const ClassData::StrDependence &dep) { return dep.class_name; };
      kphp_error(false, fmt_format("class `{}` has unresolved dependencies [{}]", c->name,
        vk::join(vk::filter(is_not_done, c->get_str_dependents()), ", ", str_dep_to_string))
      );
    }

    if (!c->is_builtin() && c->is_polymorphic_class()) {
      if (vk::any_of(c->get_all_derived_classes(), [](ClassPtr c) { return c->is_class(); })) {
        auto virt_clone = c->add_virt_clone();
        G->register_and_require_function(virt_clone, generated_self_methods, true);
      }
    }
  }

  for (ClassPtr c : classes) {
    bool is_top_of_hierarchy = !c->parent_class && c->implements.empty() && !c->derived_classes.empty();
    if (is_top_of_hierarchy) {
      mark_virtual_and_overridden_methods(c, generated_self_methods);
    }

    // check that members of this class don't conflict with parent's
    // we are in on_finish() of a sync block, all classes have been parsed and are not modified by other threads: no locks
    if (c->parent_class) {
      c->members.for_each([c](const ClassMemberInstanceField &f) {
        auto field_name = f.local_name();
        kphp_error(!c->parent_class->get_instance_field(field_name) && !c->parent_class->get_static_field(field_name),
                   fmt_format("You may not override field ${} in class {}", field_name, c->name));
      });
      c->members.for_each([c](const ClassMemberStaticField &f) {
        auto field_name = f.local_name();
        kphp_error(!c->parent_class->get_instance_field(field_name),
                   fmt_format("You may not override field ${} in class {}", field_name, c->name));
      });
      c->members.for_each([c](const ClassMemberStaticMethod &m) {
        kphp_error(!c->parent_class->get_instance_method(m.local_name()),
                   fmt_format("Cannot make non-static method {}() static", m.function->as_human_readable()));
      });
    }

    // For stable code generation of polymorphic classes body
    std::sort(c->derived_classes.begin(), c->derived_classes.end());
  }

  stage::die_if_global_errors();

  for (auto fun : generated_self_methods.flush()) {
    os << fun;
  }
}
