#pragma once

#include "compiler/function-pass.h"

/**
 * 1. Паттерн list(...) = [...] или list(...) = f() : правую часть — во временную переменную $tmp_var
 * При этом $tmp_var это op_ex_var_superlocal_inplace: её нужно объявить по месту, а не выносить в начало функции в c++
 * Соответственно, по смыслу сущность $tmp_var это array или tuple
 * 2. list(...) = $var оборачиваем в op_seq_rval { list; $var }, для поддержки while(list()=f()) / if(... && list()=f())
 */
class ConvertListAssignmentsPass : public FunctionPassBase {
public:
  string get_description() final {
    return "Process assignments to list";
  }

  VertexPtr on_exit_vertex(VertexPtr root);

private:
  static VertexPtr process_list_assignment(VertexAdaptor<op_list> list);
};
