#pragma once

#include "compiler/bicycle.h"

class AnalyzerF {
public:
  void on_finish(DataStream<FunctionPtr> &os __attribute__((unused))) {};

  void execute(FunctionPtr function, DataStream<FunctionPtr> &os);
};
