#include "compiler/pipes/sort-and-inherit-classes.h"

#include <unordered_map>

#include "common/algorithms/compare.h"
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

  VertexPtr on_enter_vertex(VertexPtr root) {
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


// если все dependents класса уже обработаны, возвращает nullptr
// если же какой-то из dependents (класс/интерфейс) ещё не обработан (его надо подождать), возвращает указатель на его
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

// делаем функцию childclassname$$localname, которая выглядит как
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

    child_class->members.add_static_method(child_function);    // пока наследование только статическое

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
                    fmt_format("Error extends {} and {}", child_class->name, parent_class->name));

  kphp_error_return(!parent_class->modifiers.is_final(),
                    fmt_format("You cannot extends final class: {}", child_class->name));

  child_class->parent_class = parent_class;

  // A::f -> B -> C -> D; для D нужно C::f$$D, B::f$$D, A::f$$D
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
    parent_class->members.for_each([&](const ClassMemberConstant &c) {
      if (auto child_const = child_class->get_constant(c.local_name())) {
        kphp_error(child_const->access == c.access,
                   fmt_format("Can't change access type for constant {} in class {}", c.local_name(), child_class->name));
      }
    });
  }

  if (child_class->parent_class) {
    AutoLocker<Lockable *> locker(&(*child_class->parent_class));
    child_class->parent_class->derived_classes.emplace_back(child_class);
  }

  kphp_error(!child_class->is_serializable && !child_class->parent_class->is_serializable, "You may not serialize polymorphic classes");
}

void SortAndInheritClassesF::inherit_class_from_interface(ClassPtr child_class, InterfacePtr interface_class) {
  kphp_error(interface_class->is_interface(),
             fmt_format("Error implements {} and {}", child_class->name, interface_class->name));

  child_class->implements.emplace_back(interface_class);

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
          kphp_error(false, fmt_format("in class: {}, you have methods collision: {}", ready_class->get_name(), method->get_human_readable_name()));
        }
      }
    };

    traits[i]->members.for_each([&](ClassMemberInstanceMethod &m) { check_other_traits_doesnt_contain_method_and_clone(m.function); });
    traits[i]->members.for_each([&](ClassMemberStaticMethod   &m) { check_other_traits_doesnt_contain_method_and_clone(m.function); });

    traits[i]->members.for_each([&](const ClassMemberInstanceField &f) {
      ready_class->members.add_instance_field(f.root.clone(), f.var->init_val.clone(), f.modifiers, f.phpdoc_str);
    });

    traits[i]->members.for_each([&](const ClassMemberStaticField &f) {
      ready_class->members.add_static_field(f.root.clone(), f.var->init_val.clone(), f.modifiers, f.phpdoc_str);
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

/**
 * Каждый класс поступает сюда один и ровно один раз — когда он и все его dependents
 * (родители, трейты, интерфейсы) тоже готовы.
 */
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
  DataStream<FunctionPtr> generated_self_methods{true};
  auto classes = G->get_classes();
  for (auto c : classes) {
    auto node = ht.at(vk::std_hash(c->name));
    kphp_error(node->data.done, fmt_format("class `{}` has unresolved dependencies", c->name));

    if (!c->is_builtin() && c->is_polymorphic_class()) {
      auto inheritors = c->get_all_inheritors();
      bool has_class_in_hierarchy = vk::any_of(inheritors, [](ClassPtr c) { return c->is_class(); });
      if (has_class_in_hierarchy) {
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
