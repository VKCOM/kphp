// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/preprocess-exceptions.h"

#include "compiler/data/src-file.h"
#include "compiler/gentree.h"

VertexPtr PreprocessExceptions::on_exit_vertex(VertexPtr root) {
  static const ClassPtr exception_class = G->get_class("Exception");

  auto call = root.try_as<op_func_call>();
  if (!call || !is_constructor_call(call)) {
    return root;
  }
  auto alloc = call->args()[0].try_as<op_alloc>();
  if (!alloc) {
    return root;
  }

  ClassPtr klass = G->get_class(alloc->allocated_class_name);
  kphp_assert(klass);
  if (!exception_class->is_parent_of(klass)) {
    return root;
  }

  // root is a call to the Exception-derived class;
  // we want to add source location info into the object being created
  //
  // new T(args...)
  // =>
  // __exception_set_location(new T(args...), __FILE__, __LINE__)

  auto file_arg = VertexAdaptor<op_string>::create().set_location(root->location);
  file_arg->set_string(root->location.get_file()->file_name);
  auto line_arg = GenTree::create_int_const(root->location.get_line());
  auto call2 = VertexAdaptor<op_func_call>::create(call, file_arg, line_arg);
  call2->set_string("__exception_set_location");
  call2.set_location(root->location);

  return call2;
}
