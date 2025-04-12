// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/preprocess-exceptions.h"

#include "compiler/data/src-file.h"
#include "compiler/vertex-util.h"
#include "compiler/compiler-core.h"

VertexPtr PreprocessExceptions::on_exit_vertex(VertexPtr root) {
  static const ClassPtr throwable_class = G->get_class("Throwable");
  if (!throwable_class) {   // when functions.txt deleted while development
    return root;
  }

  if (auto catch_op = root.try_as<op_catch>()) {
    catch_op->exception_class = G->get_class(catch_op->type_declaration);
    kphp_error_act(catch_op->exception_class,
                   fmt_format("Can't find class: {}", catch_op->type_declaration),
                   return catch_op);
    if (catch_op->exception_class->is_class()) {
      kphp_error(throwable_class->is_parent_of(catch_op->exception_class),
                 fmt_format("Can't catch {}; only classes that implement Throwable can be caught", catch_op->type_declaration));
    }
    return catch_op;
  }

  auto call = root.try_as<op_func_call>();
  if (!call || call->extra_type != op_ex_constructor_call) {
    return root;
  }
  auto alloc = call->args()[0].try_as<op_alloc>();
  if (!alloc || !alloc->allocated_class) {
    return root;
  }
  if (!throwable_class->is_parent_of(alloc->allocated_class)) {
    return root;
  }

  // file and line are passed to "new Exception" construction
  // but we are using relative file name (relative to base dir) for two purposes:
  // 1) file names on compilation servers have often /some/strange/absolute paths, strip them off
  // 2) the result of codegeneration doesn't depend on username / project location
  auto file_arg = VertexAdaptor<op_string>::create().set_location(root->location);
  file_arg->set_string(root->location.get_file()->relative_file_name);
  auto line_arg = VertexUtil::create_int_const(root->location.get_line());
  auto new_call = VertexAdaptor<op_func_call>::create(call, file_arg, line_arg).set_location(root->location);
  new_call->func_id = G->get_function("_exception_set_location");
  new_call->auto_inserted = true;
  return new_call;
}
