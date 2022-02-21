// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/class-data.h"
#include "compiler/data/class-member-modifiers.h"
#include "compiler/function-pass.h"

class CheckAccessModifiersPass final : public FunctionPassBase {
private:
  ClassPtr class_id;
  ClassPtr lambda_class_id;
public:
  std::string get_description() override {
    return "Check access modifiers";
  }

  bool check_function(FunctionPtr function) const override {
    return !function->is_extern();
  }

  void on_start() override;

  VertexPtr on_enter_vertex(VertexPtr root) override;
};

namespace impl_ {

template<class Field>
const Field &get_field_name(const Field &field) {
  return field;
}

inline std::string get_field_name(const FunctionPtr &field) {
  return field->as_human_readable();
}

} // namespace impl_

template<class MemberModifier, class Field>
void check_access(ClassPtr class_id, ClassPtr lambda_class_id, MemberModifier modifiers, ClassPtr access_class, const char *entity_type, const Field &field) {
  if (modifiers.is_private()) {
    kphp_error(class_id == access_class || lambda_class_id == access_class,
               fmt_format("Can't access private {} {}", entity_type, impl_::get_field_name(field)));
  } else if (modifiers.is_protected()) {
    auto is_ok = [&access_class](ClassPtr class_id) {
      return class_id && (class_id->is_parent_of(access_class) || access_class->is_parent_of(class_id));
    };
    kphp_error(is_ok(class_id) || is_ok(lambda_class_id),
               fmt_format("Can't access protected {} {}", entity_type, impl_::get_field_name(field)));
  }
}

