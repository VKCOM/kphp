// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/resolve-self-static-parent.h"

#include "common/wrappers/likely.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/gentree.h"
#include "compiler/name-gen.h"
#include "compiler/phpdoc.h"
#include "compiler/type-hint.h"
#include "compiler/vertex-util.h"

void ResolveSelfStaticParentPass::on_start() {
  // replace self::, parent:: and accesses to other classes like Classes\A::CONST
  if (current_function->type == FunctionData::func_class_holder) {
    current_function->class_id->members.for_each([&](ClassMemberConstant& c) { run_function_pass(c.value, this); });
    current_function->class_id->members.for_each([&](ClassMemberStaticField& f) {
      if (f.var->init_val) {
        run_function_pass(f.var->init_val, this);
      }
    });
    current_function->class_id->members.for_each([&](ClassMemberInstanceField& f) {
      if (f.var->init_val) {
        run_function_pass(f.var->init_val, this);
      }
    });
  }
}

VertexPtr ResolveSelfStaticParentPass::on_enter_vertex(VertexPtr v) {
  if (auto as_func_call = v.try_as<op_func_call>()) {
    // replace \VK\A::method() with VK$A$$method(), also replace parent::method() with $this->method()
    auto pos = as_func_call->str_val.find("::");
    if (pos == std::string::npos) {
      return v;
    }

    const std::string& prefix_name = as_func_call->str_val.substr(0, pos);
    const std::string& local_name = as_func_call->str_val.substr(pos + 2);
    const std::string& class_name = resolve_uses(current_function, prefix_name);
    ClassPtr ref_class = G->get_class(class_name);
    check_access_to_class_from_this_file(prefix_name, ref_class);

    // PHP allows to call self::instance_method(), we should replace it with $this->instance_method()
    // but don't replace SomeOtherClass::instance_method(), it should give an error "non-static method is called statically"
    bool inside_instance_method = current_function->get_this_or_topmost_if_lambda()->modifiers.is_instance();
    if (ref_class && inside_instance_method && ref_class->is_parent_of(current_function->get_this_or_topmost_if_lambda()->class_id)) {
      if (const auto* found_method = ref_class->get_instance_method(local_name)) {
        if (!found_method->function->modifiers.is_abstract()) {
          return replace_func_call_with_colons_with_this_call(found_method->function, ref_class, as_func_call);
        }
      }
    }

    as_func_call->str_val = resolve_static_method_append_context(current_function, prefix_name, class_name, local_name);

  } else if (auto as_var = v.try_as<op_var>()) {
    // replace Klass::prop with Klass$$prop
    auto pos = as_var->str_val.find("::");
    if (pos == std::string::npos) {
      if (current_function->is_lambda() && as_var->str_val == "this") {
        return GenTree::auto_capture_this_in_lambda(current_function);
      }
      return v;
    }

    const std::string& prefix_name = as_var->str_val.substr(0, pos);
    const std::string& var_name = as_var->str_val.substr(pos + 2);
    const std::string& class_name = resolve_uses(current_function, prefix_name);
    ClassPtr ref_class = G->get_class(class_name);
    check_access_to_class_from_this_file(prefix_name, ref_class);

    as_var->str_val = class_name + "$$" + var_name;

  } else if (auto as_func_name = v.try_as<op_func_name>()) {
    // replace Klass::CONSTANT with Klass$$CONSTANT
    auto pos = as_func_name->str_val.find("::");
    if (pos == std::string::npos) {
      return v;
    }

    const std::string& prefix_name = as_func_name->str_val.substr(0, pos);
    const std::string& const_name = as_func_name->str_val.substr(pos + 2);
    const std::string& class_name = resolve_uses(current_function, prefix_name);
    ClassPtr ref_class = G->get_class(class_name);
    check_access_to_class_from_this_file(prefix_name, ref_class);

    as_func_name->str_val = class_name + "$$" + const_name;

  } else if (auto as_alloc = v.try_as<op_alloc>()) {
    // replace 'new A' to the fully resolved name 'new Classes\A' (set allocated_class)
    const std::string& prefix_name = as_alloc->allocated_class_name;
    const std::string& class_name = resolve_uses(current_function, prefix_name);
    ClassPtr ref_class = G->get_class(class_name);
    check_access_to_class_from_this_file(prefix_name, ref_class);

    as_alloc->allocated_class = ref_class; // if not exists, checked later, on func_call bind

  } else if (auto as_instanceof = v.try_as<op_instanceof>()) {
    // ... instanceof XXX, the right was replaced by XXX::class
    const std::string& instanceof_class = VertexUtil::get_actual_value(as_instanceof->rhs())->get_string();
    auto pos = instanceof_class.find("::");
    kphp_assert(pos != std::string::npos);

    const std::string& prefix_name = instanceof_class.substr(0, pos);
    const std::string& class_name = resolve_uses(current_function, prefix_name);
    ClassPtr ref_class = G->get_class(class_name);
    check_access_to_class_from_this_file(prefix_name, ref_class);

    as_instanceof->derived_class = ref_class;
    kphp_error(ref_class, "Unknown class at the right of 'instanceof'");
  }

  return v;
}

void ResolveSelfStaticParentPass::check_access_to_class_from_this_file(const std::string& prefix_name, ClassPtr ref_class) {
  bool inside_instance_method = current_function->get_this_or_topmost_if_lambda()->modifiers.is_instance();

  if (inside_instance_method && prefix_name == "static") {
    kphp_error(ref_class && ref_class->derived_classes.empty(), "Using 'static' is prohibited from instance methods when a class has derived classes.\nEither "
                                                                "use 'self', or modify your code to use '$this->' func calls.");
  }
  if (ref_class && !ref_class->can_be_php_autoloaded && !ref_class->file_id->is_loaded_by_psr0) {
    // todo temporary allow direct using tl _result classes for vkcom; roll back after switching to long ID
    kphp_error(ref_class->file_id == current_function->file_id || current_function->is_virtual_method ||
                   (ref_class->phpdoc && ref_class->phpdoc->has_tag(PhpDocType::kphp_tl_class)),
               fmt_format("Class {} can be accessed only from file {}, as it is not autoloadable", ref_class->name, ref_class->file_id->relative_file_name));
  }
  if (ref_class && ref_class->is_trait()) {
    kphp_error(current_function->class_id && current_function->class_id->is_trait(),
               fmt_format("You may not use trait directly: {}", ref_class->as_human_readable()));
  }
}

// PHP allows to call self::instance_method(), we should replace it with $this->instance_method()
// (parent::fun() follows the same logic, ref_class refs to a parent class, considering that fun() is virtual in parent)
VertexPtr ResolveSelfStaticParentPass::replace_func_call_with_colons_with_this_call(FunctionPtr called_method, ClassPtr ref_class,
                                                                                    VertexAdaptor<op_func_call> call_colons) {
  VertexPtr this_vertex =
      current_function->is_lambda() ? GenTree::auto_capture_this_in_lambda(current_function) : ClassData::gen_vertex_this(call_colons->location);

  auto call_inst = VertexAdaptor<op_func_call>::create(this_vertex, call_colons->args()).set_location(call_colons);
  call_inst->extra_type = op_ex_func_call_arrow;
  call_inst->str_val = std::string{called_method->local_name()};
  call_inst->func_id = called_method;

  if (called_method->is_virtual_method) {
    const auto* self_method = ref_class->get_instance_method(called_method->get_name_of_self_method());
    kphp_assert(self_method);
    call_inst->str_val = std::string{self_method->local_name()};
    call_inst->func_id = self_method->function;
  }

  return call_inst;
}
