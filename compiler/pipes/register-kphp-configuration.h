#pragma once

#include "compiler/function-pass.h"

class RegisterKphpConfiguration final : public FunctionPassBase {
public:
  string get_description() final {
    return "Register KPHP Configuration";
  }

  bool check_function(FunctionPtr function) const final;
  bool user_recursion(VertexPtr) final { return true; }
  void on_start() final;

private:
  const vk::string_view configuration_class_name_{"KphpConfiguration"};
};
