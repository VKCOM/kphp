#pragma once

#include "compiler/function-pass.h"

class CheckTlClasses final : public FunctionPassBase {
public:
  string get_description() override {
    return "Check tl classes";
  }

  bool check_function(FunctionPtr function) const override;
  bool on_start(FunctionPtr function) override;
};
