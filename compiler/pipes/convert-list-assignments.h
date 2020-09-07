#pragma once

#include "compiler/function-pass.h"

// This pipe rewrites some list assignments.
//
// 1. In list(...) = [...] or list(...) = f() we put the RHS into the $tmp_var variable.
// This $tmp_var is op_ex_var_superlocal_inplace: it should be declared right before its usage
// as opposed to be declared at the top of the function body.
// $tmp_var contains array or tuple value.
//
// 2. list(...) = $var is wrapped into the op_seq_rval { list; $var } to support
// 'while (list(...) = f())' and 'if (... && list(...) = f())'
class ConvertListAssignmentsPass final : public FunctionPassBase {
public:
  string get_description() final {
    return "Process assignments to list";
  }

  VertexPtr on_exit_vertex(VertexPtr root) override;

private:
  static VertexPtr process_list_assignment(VertexAdaptor<op_list> list);
};
