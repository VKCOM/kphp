#pragma once

#include <forward_list>

#include "compiler/data/class-members.h"
#include "compiler/data/data_ptr.h"
#include "compiler/threading/data-stream.h"
#include "compiler/threading/hash-table.h"

class SortAndInheritClassesF {
private:
  struct wait_list {
    bool done;
    std::forward_list<ClassPtr> waiting;
  };

  static TSHashTable<wait_list> ht;

  void on_class_ready(ClassPtr klass, DataStream<FunctionPtr> &function_stream);
  void analyze_class_phpdoc(ClassPtr klass);

  VertexAdaptor<op_function> generate_function_with_parent_call(ClassPtr child_class, const ClassMemberStaticMethod &parent_method);
  FunctionPtr create_function_with_context(FunctionPtr parent_f, const std::string &ctx_function_name);

  void inherit_child_class_from_parent(ClassPtr child_class, ClassPtr parent_class, DataStream<FunctionPtr> &function_stream);
  void inherit_static_method_from_parent(ClassPtr child_class, const ClassMemberStaticMethod &parent_method, DataStream<FunctionPtr> &function_stream);

  void inherit_class_from_interface(ClassPtr child_class, InterfacePtr interface_class);
  static void clone_members_from_traits(std::vector<TraitPtr> &&traits, ClassPtr ready_class, DataStream<FunctionPtr> &function_stream);

  decltype(ht)::HTNode *get_not_ready_dependency(ClassPtr klass);

public:
  void execute(ClassPtr klass, MultipleDataStreams<FunctionPtr, ClassPtr> &os);
  static void check_on_finish(DataStream<FunctionPtr> &os);
};
