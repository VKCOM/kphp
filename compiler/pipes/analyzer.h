#pragma once

#include "compiler/bicycle.h"

class AnalyzerF {
public:
  void execute(FunctionPtr function, DataStream<FunctionPtr> &os);
};
