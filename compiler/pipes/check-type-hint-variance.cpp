// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/check-type-hint-variance.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/type-hint.h"

#include "common/containers/final_action.h"

namespace {

FunctionPtr get_derived_method(FunctionPtr base_function, ClassPtr derived_class) {
  if (base_function->modifiers.is_static()) {
    auto* member = derived_class->members.get_static_method(base_function->local_name());
    return member ? member->function : FunctionPtr{};
  }
  auto* member = derived_class->members.get_instance_method(base_function->local_name());
  return member ? member->function : FunctionPtr{};
}

} // namespace

void CheckTypeHintVariance::execute(FunctionPtr f, DataStream<FunctionPtr>& os) {
  stage::set_name("Child method signature check");
  stage::set_function(f);
  stage::set_line(f->root->get_location().line);

  auto forward_function_next = vk::finally([&] { os << f; });

  if (!f->class_id) {
    return;
  }
  if (f->modifiers.is_final() || f->class_id->derived_classes.empty()) {
    return;
  }

  // constructors don't follow the same variance rules;
  // methods from functions.txt may use type hints like `mixed` that can't
  // be expressed in PHP7.4, so we skip it for now
  if (f->is_constructor() || f->is_extern()) {
    return;
  }

  // for every non-final class method check that derived classes override
  // it carefully, so PHP won't give a fatal error about mismatching typehints;
  //
  // we do an actual type checking in GenerateVirtualMethods pass

  for (const auto& derived_class : f->class_id->derived_classes) {
    const FunctionPtr derived_f = get_derived_method(f, derived_class);
    if (!derived_f) {
      continue;
    }

    // if a base method has a return type hint, derived method should have it too
    kphp_error(!f->return_typehint || derived_f->return_typehint,
               fmt_format("{}() should provide a return type hint compatible with {}()", derived_f->as_human_readable(), f->as_human_readable()));

    // if base methods doesn't have a param type hint, derived method can't have it too
    const auto& params = f->get_params();
    const auto& derived_params = derived_f->get_params();
    if (params.size() != derived_params.size()) {
      continue;
    }
    int first_param = f->modifiers.is_static() ? 0 : 1;
    for (int i = first_param; i < params.size(); ++i) {
      auto p = params[i].as<op_func_param>();
      auto derived_p = derived_params[i].as<op_func_param>();
      if (!p->type_hint) {
        kphp_error(!derived_p->type_hint, fmt_format("{}() should not have a ${} param type hint, it would be incompatible with {}()",
                                                     derived_f->as_human_readable(), derived_p->var()->get_string(), f->as_human_readable()));
      }
    }
  }
}
