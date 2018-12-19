#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/threading/data-stream.h"

class CloneParentMethodWithContextF {
  void create_ast_of_function_with_context(FunctionPtr function, DataStream<FunctionPtr> &os);

public:
  void execute(FunctionPtr function, DataStream<FunctionPtr> &os);
};
