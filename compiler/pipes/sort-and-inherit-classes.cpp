// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/sort-and-inherit-classes.h"

#include <unordered_map>

#include "common/algorithms/compare.h"
#include "common/algorithms/contains.h"
#include "common/algorithms/hashes.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/function-pass.h"
#include "compiler/gentree.h"
#include "compiler/phpdoc.h"
#include "compiler/stage.h"
#include "compiler/threading/profiler.h"
#include "compiler/utils/string-utils.h"

// This pass takes functions like baseclassname$$fname$$contextclassname
// that were cloned from the parent tree.
//
// This pass should go away in the future when lambdas will be handled
// after the gentree (not during it).
class PatchInheritedMethodPass final : public FunctionPassBase {
  DataStream<FunctionPtr> &function_stream;
public:
  string get_description() override {
    return "Patch inherited methods";
  }
  explicit PatchInheritedMethodPass(DataStream<FunctionPtr> &function_stream) :
    function_stream(function_stream) {}

  VertexPtr on_enter_vertex(VertexPtr root) override {
    if (auto call = root.try_as<op_func_call>()) {
      if (!call->args().empty()) {
        if (auto alloc = call->args()[0].try_as<op_alloc>()) {
          if (auto lambda_class = alloc->allocated_class.try_as<LambdaClassData>()) {
            FunctionPtr invoke_method = lambda_class->members.get_instance_method(ClassData::NAME_OF_INVOKE_METHOD)->function;
            vector<VertexAdaptor<op_func_param>> uses_of_lambda;

            lambda_class->members.for_each([&](ClassMemberInstanceField &f) {
              auto new_var_use = VertexAdaptor<op_var>::create().set_location(f.root);
              new_var_use->set_string(std::string{f.local_name()});
              auto func_param = VertexAdaptor<op_func_param>::create(new_var_use).set_location(f.root);

              uses_of_lambda.insert(uses_of_lambda.begin(), func_param);
            });

            return GenTree::generate_anonymous_class(invoke_method->root, current_function, true, std::move(uses_of_lambda))
              .require(function_stream)
              .get_generated_lambda()
              ->gen_constructor_call_pass_fields_as_args();
          }
        }
      }
    }

    return root;
  }
};

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
      if (auto parent_member = ancestor->members.get_instance_method(method.local_name())) {
        auto parent_function = parent_member->function;

        kphp_error(parent_function->modifiers.is_abstract() || !parent_function->modifiers.is_final(),
                   fmt_format("You cannot override final method: {}", method.function->get_human_readable_name()));

        kphp_error(parent_function->modifiers.access_modifier() == method.function->modifiers.access_modifier(),
                   fmt_format("Can not change access type for method: {}", method.function->get_human_readable_name()));

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
                                               (!std::is_same<ClassMemberT, ClassMemberStaticField>{} && par->get_static_field(field_name));
    kphp_error(!parent_has_instance_or_static_field,
               fmt_format("You may not override field: `{}`, in class: `{}`", field_name, c->name));
  }
};

bool clone_method(FunctionPtr from, ClassPtr to_class, DataStream<FunctionPtr> &function_stream) {
  if (from->modifiers.is_instance() && to_class->members.has_instance_method(from->local_name())) {
    return false;
  } else if (from->modifiers.is_static() && to_class->members.has_static_method(from->local_name())) {
    return false;
  }

  auto new_root = from->root.clone();
  auto cloned_fun = FunctionData::clone_from(replace_backslashes(to_class->name) + "$$" + from->local_name(), from, new_root);
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
    auto node = ht.at(vk::std_hash(dep.class_name));
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
  auto func = VertexAdaptor<op_function>::create(new_param_list, new_cmd);
  func->copy_location_and_flags(*parent_f->root);

  return func;
}

// Clone the baseclassname$$fname function into the contextual baseclassname$$fname$$contextclassname.
// Contextual functions are needed for the static inheritance implementation.
FunctionPtr SortAndInheritClassesF::create_function_with_context(FunctionPtr parent_f, const std::string &ctx_function_name) {
  auto root = parent_f->root.clone();
  auto context_function = FunctionData::clone_from(ctx_function_name, parent_f, root);

  return context_function;
}

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

  if (auto child_method = child_class->members.get_static_method(local_name)) {
    kphp_error_return(!parent_f->modifiers.is_final(),
                      fmt_format("Can not override method marked as 'final': {}", parent_f->get_human_readable_name()));
    kphp_error_return(!(parent_method.function->modifiers.is_static() && parent_method.function->modifiers.is_private()),
                      fmt_format("Can not override private method: {}", parent_f->get_human_readable_name()));
    kphp_error_return(parent_method.function->modifiers.access_modifier() == child_method->function->modifiers.access_modifier(),
                      fmt_format("Can not change access type for method: {}", child_method->function->get_human_readable_name()));
  } else {
    auto child_root = generate_function_with_parent_call(child_class, parent_method);

    string new_name = replace_backslashes(child_class->name) + "$$" + parent_method.local_name();
    FunctionPtr child_function = FunctionData::clone_from(new_name, parent_f, child_root);
    child_function->is_auto_inherited = true;
    child_function->is_inline = true;

    // only static inheritance is supported right now
    child_class->members.add_static_method(child_function);

    G->register_and_require_function(child_function, function_stream);
  }

  // contextual function has a name like baseclassname$$fname$$contextclassname;
  // it's a cloned tree of baseclassname$$fname (parent_f) + kludge for lambdas (which should eventually go away)
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
    parent_class->members.for_each([&](const ClassMemberStaticField &f) {
      if (auto field = child_class->members.get_static_field(f.local_name())) {
        kphp_error(!f.modifiers.is_private(),
                   fmt_format("Can't redeclare private static field {} in class {}\n", f.local_name(), child_class->name));

        kphp_error(f.modifiers == field->modifiers,
                   fmt_format("Can't change access type for static field {} ({}) in class {} ({})\n",
                              f.local_name(), f.modifiers.to_string(),
                              child_class->name, field->modifiers.to_string())
        );
      }
    });
    parent_class->members.for_each([&](ClassMemberConstant &parent_const) {
      if (auto child_const = child_class->get_constant(parent_const.local_name())) {
        check_const_inheritance(child_class, parent_class, &parent_const, child_const);
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
             fmt_format("Error implements {} and {}", child_class->name, interface_class->name));

  child_class->implements.emplace_back(interface_class);

  interface_class->members.for_each([&](const ClassMemberConstant &parent_const) {
    if (auto child_const = child_class->get_constant(parent_const.local_name())) {
      check_const_inheritance(child_class, interface_class, &parent_const, child_const);
    }
  });

  AutoLocker<Lockable *> locker(&(*interface_class));
  interface_class->derived_classes.emplace_back(child_class);
}

void SortAndInheritClassesF::check_const_inheritance(ClassPtr child_class, ClassPtr parent_class, const  ClassMemberConstant *parent_const, const ClassMemberConstant *child_const) {
  if (parent_const->is_final && child_const->klass == child_class) {
    const auto child_name = child_class->name + "::" + child_const->local_name();
    const auto parent_name = parent_class->name + "::" + parent_const->local_name();
    kphp_error(0, fmt_format("{} cannot override final constant {}", child_name, parent_name));
  }

  kphp_error(child_const->access == parent_const->access,
             fmt_format("Can't change access type for constant {} in class {}", parent_const->local_name(), child_class->name));
}

void SortAndInheritClassesF::clone_members_from_traits(std::vector<TraitPtr> &&traits, ClassPtr ready_class, DataStream<FunctionPtr> &function_stream) {
  for (size_t i = 0; i < traits.size(); ++i) {
    auto check_other_traits_doesnt_contain_method_and_clone = [&](FunctionPtr method) {
      if (!clone_method(method, ready_class, function_stream)) {
        return;
      }

      for (size_t j = i + 1; j < traits.size(); ++j) {
        if (traits[j]->members.has_instance_method(method->local_name()) || traits[j]->members.has_static_method(method->local_name())) {
          kphp_error(false, fmt_format("in class: {}, you have methods collision: {}", ready_class->get_name(), method->get_human_readable_name()));
        }
      }
    };

    traits[i]->members.for_each([&](ClassMemberInstanceMethod &m) { check_other_traits_doesnt_contain_method_and_clone(m.function); });
    traits[i]->members.for_each([&](ClassMemberStaticMethod   &m) { check_other_traits_doesnt_contain_method_and_clone(m.function); });

    traits[i]->members.for_each([&](const ClassMemberInstanceField &f) {
      ready_class->members.add_instance_field(f.root.clone(), f.var->init_val.clone(), f.modifiers, f.phpdoc_str, f.type_hint);
    });

    traits[i]->members.for_each([&](const ClassMemberStaticField &f) {
      ready_class->members.add_static_field(f.root.clone(), f.var->init_val.clone(), f.modifiers, f.phpdoc_str, f.type_hint);
    });
  }

  if (ready_class->is_class()) {
    ready_class->members.for_each([&](ClassMemberInstanceMethod &m) {
      if (m.function->modifiers.is_abstract()) {
        kphp_error(ready_class->modifiers.is_abstract(), fmt_format("class: {} must be declared abstract, because of abstract method: {}",
                                                                    ready_class->get_name(), m.function->get_human_readable_name()));
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

  if (!klass->phpdoc_str.empty()) {
    analyze_class_phpdoc(klass);
  }
}

inline void SortAndInheritClassesF::analyze_class_phpdoc(ClassPtr klass) {
  if (phpdoc_tag_exists(klass->phpdoc_str, php_doc_tag::kphp_memcache_class)) {
    G->set_memcache_class(klass);
  }
}

void SortAndInheritClassesF::execute(ClassPtr klass, MultipleDataStreams<FunctionPtr, ClassPtr> &os) {
  auto &function_stream = *os.template project_to_nth_data_stream<0>();
  auto &restart_class_stream = *os.template project_to_nth_data_stream<1>();

  if (auto dependency = get_not_ready_dependency(klass)) {
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

  auto node = ht.at(vk::std_hash(klass->name));
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
  for (auto c : classes) {
    auto node = ht.at(vk::std_hash(c->name));
    if (!node->data.done) {
      auto is_not_done = [](const ClassData::StrDependence &dep) { return !ht.at(vk::std_hash(dep.class_name))->data.done; };
      auto str_dep_to_string = [](const ClassData::StrDependence &dep) { return dep.class_name; };
      kphp_error(false, fmt_format("class `{}` has unresolved dependencies [{}]", c->name,
        vk::join(vk::filter(is_not_done, c->get_str_dependents()), ", ", str_dep_to_string))
      );
    }

    if (!c->is_builtin() && c->is_polymorphic_class()) {
      if (vk::any_of(c->get_all_inheritors(), [](ClassPtr c) { return c->is_class(); })) {
        auto virt_clone = c->add_virt_clone();
        G->register_and_require_function(virt_clone, generated_self_methods, true);
      }
    }
  }

  for (auto c : classes) {
    bool is_top_of_hierarchy = !c->parent_class && c->implements.empty() && !c->derived_classes.empty();
    if (is_top_of_hierarchy) {
      mark_virtual_and_overridden_methods(c, generated_self_methods);
    }

    if (c->parent_class) {
      c->members.for_each(CheckParentDoesntHaveMemberWithSameName<ClassMemberInstanceField>{c});
      c->members.for_each(CheckParentDoesntHaveMemberWithSameName<ClassMemberStaticField>{c});

      c->members.for_each([&c](const ClassMemberStaticMethod &m) {
        kphp_error(!c->parent_class->get_instance_method(m.local_name()),
                   fmt_format("Cannot make non static method `{}` static", m.function->get_human_readable_name()));
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
