// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/resolve-self-static-parent.h"

#include "common/wrappers/likely.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/lambda-class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/gentree.h"
#include "compiler/name-gen.h"
#include "compiler/phpdoc.h"
#include "compiler/type-hint.h"


void ResolveSelfStaticParentPass::on_start() {
  // replace self::, parent:: and accesses to other classes like Classes\A::CONST
  if (current_function->type == FunctionData::func_class_holder) {
    current_function->class_id->members.for_each([&](ClassMemberConstant &c) {
      run_function_pass(c.value, this);
    });
    current_function->class_id->members.for_each([&](ClassMemberStaticField &f) {
      if (f.var->init_val) {
        run_function_pass(f.var->init_val, this);
      }
    });
    current_function->class_id->members.for_each([&](ClassMemberInstanceField &f) {
      if (f.var->init_val) {
        run_function_pass(f.var->init_val, this);
      }
    });
  }
}

VertexPtr ResolveSelfStaticParentPass::on_enter_vertex(VertexPtr v) {
  // replace \VK\A::method, static::f, self::$field to their full names like VK$A$$method
  if (vk::any_of_equal(v->type(), op_func_call, op_var, op_func_name)) {
    auto pos = v->get_string().find("::");
    if (pos == string::npos) {
      return v;
    }
    string original_name = v->get_string();
    auto unresolved_class_name = original_name.substr(0, pos);
    const std::string &class_name = resolve_uses(current_function, unresolved_class_name, '\\');
    ClassPtr ref_class = G->get_class(class_name);
    check_access_to_class_from_this_file(ref_class);

    if (ref_class && ref_class->is_trait()) {
      kphp_error(current_function->class_id && current_function->class_id->is_trait(), fmt_format("you may not use trait directly: {}", ref_class->get_name()));
    }

    if (ref_class && current_function->modifiers.is_instance()) {
      if (auto found_method = ref_class->get_instance_method(original_name.substr(pos + 2))) {
        auto method = found_method->function;
        kphp_error(ref_class->is_parent_of(current_function->get_this_or_topmost_if_lambda()->class_id),
          fmt_format("Call of instance method({}) statically", method->get_human_readable_name()));

        VertexPtr this_vertex = ClassData::gen_vertex_this(v->location);
        if (current_function->is_lambda()) {
          this_vertex = VertexAdaptor<op_instance_prop>::create(this_vertex).set_location(v);
          this_vertex->set_string(LambdaClassData::get_parent_this_name());
        }
        v = VertexAdaptor<op_func_call>::create(this_vertex, v->get_next()).set_location(v);
        v->extra_type = op_ex_func_call_arrow;
        v->set_string(std::string{method->local_name()});
        v.as<op_func_call>()->func_id = method;

        if (unresolved_class_name != "static" && found_method->function->is_virtual_method) {
          kphp_error(current_function->modifiers.is_instance(), "calling non-static function through static");
          if (auto self_found_method = ref_class->get_instance_method(found_method->function->get_name_of_self_method())) {
            v->set_string(std::string{self_found_method->local_name()});
            v.as<op_func_call>()->func_id = self_found_method->function;
          } else {
            kphp_error(!found_method->function->modifiers.is_abstract(),
              fmt_format("you cannot call abstract methods: {}", found_method->function->get_human_readable_name()));
          }
        }
        return v;
      }
    }

    if (is_string_self_static_parent(unresolved_class_name) && !current_function->modifiers.is_static()) {
      auto field_name = original_name.substr(pos + 2);
      bool get_field_using_self_or_parent = unresolved_class_name != "static" &&
                                            (ref_class->get_static_field(field_name) || ref_class->get_constant(field_name));
      bool inside_lambda_in_static_method = current_function->function_in_which_lambda_was_created &&
                                            current_function->function_in_which_lambda_was_created->modifiers.is_static();
      kphp_error_act(ref_class->derived_classes.empty() || get_field_using_self_or_parent || inside_lambda_in_static_method,
                     "using `self/parent/static` is prohibited in non static methods", return v);
    }

    v->set_string(get_full_static_member_name(current_function, original_name, v->type() == op_func_call));
  } else if (auto alloc = v.try_as<op_alloc>()) {
    // replace 'new A' to the fully resolved name 'new Classes\A'
    if (!alloc->allocated_class) {
      alloc->allocated_class_name = resolve_uses(current_function, alloc->allocated_class_name, '\\');
      alloc->allocated_class = G->get_class(alloc->allocated_class_name);
      kphp_error_act(alloc->allocated_class, fmt_format("Class {} not found", alloc->allocated_class_name), return v);
      kphp_error_act(!alloc->allocated_class->modifiers.is_abstract(),
                     fmt_format("Cannot instantiate abstract class {}", alloc->allocated_class_name), return v);
      check_access_to_class_from_this_file(alloc->allocated_class);
    }

  } else if (auto phpdoc_var = v.try_as<op_phpdoc_var>()) {
    // replace 'self' and others if exist in @var, check classes existence
    phpdoc_var->type_hint = phpdoc_finalize_type_hint_and_resolve(phpdoc_var->type_hint, current_function);
    kphp_error(phpdoc_var->type_hint, fmt_format("Failed to parse @var inside {}", current_function->get_human_readable_name()));
  }

  return v;
}

inline void ResolveSelfStaticParentPass::check_access_to_class_from_this_file(ClassPtr ref_class) {
  if (current_function->is_virtual_method) {
    return;
  }
  if (ref_class && !ref_class->can_be_php_autoloaded) {
    kphp_error(ref_class->file_id == current_function->file_id,
               fmt_format("Class {} can be accessed only from file {}, as it is not autoloadable",
                          ref_class->name, ref_class->file_id->unified_file_name));
  }
}
