#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/threading/data-stream.h"

/*
 * Для всех функций, всех переменных проверяем, что если делались предположения насчёт классов, то они совпали с выведенными.
 * Также анализируем переменные-члены инстансов, как они вывелись.
 */
class CheckInferredInstancesF {
  void analyze_function_vars(FunctionPtr function);

  inline void analyze_function_var(FunctionPtr function, VarPtr var);

public:
  void execute(FunctionPtr function, DataStream<FunctionPtr> &os);
};
