// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

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

  void on_class_ready(ClassPtr klass, DataStream<FunctionPtr>& function_stream);

  VertexAdaptor<op_function> generate_function_with_parent_call(ClassPtr child_class, const ClassMemberStaticMethod& parent_method);

  void inherit_child_class_from_parent(ClassPtr child_class, ClassPtr parent_class, DataStream<FunctionPtr>& function_stream);
  void inherit_static_method_from_parent(ClassPtr child_class, const ClassMemberStaticMethod& parent_method, DataStream<FunctionPtr>& function_stream);
  void inherit_instance_method_from_parent(ClassPtr child_class, const ClassMemberInstanceMethod& parent_method);

  void inherit_class_from_interface(ClassPtr child_class, InterfacePtr interface_class);
  static void clone_members_from_traits(std::vector<TraitPtr>&& traits, ClassPtr ready_class, DataStream<FunctionPtr>& function_stream);

  decltype(ht)::HTNode* get_not_ready_dependency(ClassPtr klass);

public:
  void execute(ClassPtr klass, MultipleDataStreams<FunctionPtr, ClassPtr>& os);
  static void check_on_finish(DataStream<FunctionPtr>& os);
};
