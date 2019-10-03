#pragma once

#include "compiler/data/class-member-modifiers.h"
#include "compiler/function-pass.h"

class CheckAccessModifiersPass : public FunctionPassBase {
private:
  string namespace_name;
  string class_name;
  ClassPtr class_id;
  ClassPtr lambda_class_id;
  template<class MemberModifier>
  void check_access(MemberModifier modifiers, ClassPtr access_class, const char *field_type, vk::string_view name);
public:
  string get_description() {
    return "Check access modifiers";
  }

  bool check_function(FunctionPtr function) {
    return default_check_function(function) && !function->is_extern();
  }

  bool on_start(FunctionPtr function);

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *);
};
