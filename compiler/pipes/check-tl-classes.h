#pragma once

#include "compiler/function-pass.h"

class CheckTlClasses : public FunctionPassBase {
public:
  string get_description() {
    return "Check tl classes";
  }

  bool on_start(FunctionPtr function);
};
