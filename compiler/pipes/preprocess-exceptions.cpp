// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/preprocess-exceptions.h"

#include "compiler/data/src-file.h"
#include "compiler/gentree.h"

VertexPtr PreprocessExceptions::on_exit_vertex(VertexPtr root) {
  static const ClassPtr throwable_class = G->get_class("Throwable");

  if (auto catch_op = root.try_as<op_catch>()) {
    catch_op->exception_class = G->get_class(catch_op->type_declaration);
    kphp_error(catch_op->exception_class, fmt_format("Can't find class: {}", catch_op->type_declaration));
    return catch_op;
  }

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
  if (!throwable_class->is_parent_of(klass)) {
    return root;
  }

  auto file_arg = VertexAdaptor<op_string>::create().set_location(root->location);
  file_arg->set_string(root->location.get_file()->file_name);
  auto line_arg = GenTree::create_int_const(root->location.get_line());
  return VertexAdaptor<op_exception_constructor_call>::create(call, file_arg, line_arg).set_location(root->location);
}
