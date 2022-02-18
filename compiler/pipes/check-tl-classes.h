// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class CheckTlClasses final : public FunctionPassBase {
public:
  std::string get_description() override {
    return "Check tl classes";
  }

  bool check_function(FunctionPtr function) const override;
  void on_start() override;
};
