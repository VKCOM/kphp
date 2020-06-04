#pragma once

#include "compiler/data/class-data.h"
#include "compiler/data/class-member-modifiers.h"
#include "compiler/function-pass.h"

class CheckAccessModifiersPass final : public FunctionPassBase {
private:
  string namespace_name;
  string class_name;
  ClassPtr class_id;
  ClassPtr lambda_class_id;
public:
  string get_description() override {
    return "Check access modifiers";
  }

  bool check_function(FunctionPtr function) const override {
    return !function->is_extern();
  }

  bool on_start(FunctionPtr function) override;

  VertexPtr on_enter_vertex(VertexPtr root) override;
};

template<class MemberModifier>
void check_access(ClassPtr class_id, ClassPtr lambda_class_id, MemberModifier modifiers, ClassPtr access_class, const char *entity_type, vk::string_view name) {
  if (modifiers.is_private()) {
    kphp_error(class_id == access_class || lambda_class_id == access_class, fmt_format("Can't access private {} {}", entity_type, name));
  } else if (modifiers.is_protected()) {
    auto is_ok = [&access_class](ClassPtr class_id) {
      return class_id && (class_id->is_parent_of(access_class) || access_class->is_parent_of(class_id));
    };
    kphp_error(is_ok(class_id) || is_ok(lambda_class_id),
               fmt_format("Can't access protected {} {}", entity_type, name));
  }
}

