#pragma once

#include "compiler/bicycle.h"

/*
 * Для всех функций, всех переменных проверяем, что если делались предположения насчёт классов, то они совпали с выведенными.
 * Также анализируем переменные-члены инстансов, как они вывелись.
 */
class CheckInferredInstances {
  void analyze_function_vars(FunctionPtr function);

  inline void analyze_function_var(FunctionPtr function, VarPtr var);

  void analyze_class(ClassPtr klass);

public:
  void execute(FunctionPtr function, DataStream<FunctionPtr> &os);
};
