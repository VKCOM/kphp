#pragma once

#include <mutex>
#include <list>

#include "compiler/data/data_ptr.h"
#include "compiler/threading/data-stream.h"

class CollectRequiredAndClassesF {
  std::list<ClassPtr> classes_waiting;
  std::mutex mutex_classes_waiting;

  void on_class_executing(ClassPtr klass, DataStream<FunctionPtr> &function_stream);

  bool is_class_ready(ClassPtr klass);
  void on_class_ready(ClassPtr klass, DataStream<FunctionPtr> &function_stream);

  void inherit_child_class_from_parent(ClassPtr child_class, ClassPtr parent_class, DataStream<FunctionPtr> &function_stream);
  void inherit_method_from_parent_class(ClassPtr child_class, ClassPtr parent_class, const string &local_name, DataStream<FunctionPtr> &function_stream);

public:
  using ReadyFunctionPtr = FunctionPtr;
  using OStreamT = MultipleDataStreams<ReadyFunctionPtr, SrcFilePtr, FunctionPtr>;

  void execute(FunctionPtr function, OStreamT &os);
};
