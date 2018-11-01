#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/threading/data-stream.h"

/*
 * Пайп, который после inferring'а бежит по всем классам и что-то проверяет.
 */
class CheckClassesF {
  void analyze_class(ClassPtr klass);

  void check_instance_fields_inited(ClassPtr klass);

  void check_static_fields_inited(ClassPtr klass);

public:
  void execute(FunctionPtr function, DataStream<FunctionPtr> &os);
};
