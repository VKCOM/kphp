#pragma once

#include "compiler/function-pass.h"

/**
 * 1. Паттерн list(...) = [...] или list(...) = f() : правую часть — во временную переменную $tmp_var
 * При этом $tmp_var это op_ex_var_superlocal_inplace: её нужно объявить по месту, а не выносить в начало функции в c++
 * Соответственно, по смыслу сущность $tmp_var это array или tuple
 * 2. list(...) = $var оборачиваем в op_seq_rval { list; $var }, для поддержки while(list()=f()) / if(... && list()=f())
 */
class ConvertListAssignmentsPass : public FunctionPassBase {
  VertexPtr process_list_assignment(VertexAdaptor<op_list> list);
public:
  struct LocalT : public FunctionPassBase::LocalT {
    bool need_recursion_flag = true;
  };

  string get_description() {
    return "Process assignments to list";
  }

  bool check_function(FunctionPtr function) {
    return default_check_function(function) && !function->is_extern();
  }

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *local);

  bool need_recursion(VertexPtr root __attribute__ ((unused)), LocalT *local) {
    return local->need_recursion_flag;
  }

};
