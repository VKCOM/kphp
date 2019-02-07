#include "compiler/pipes/convert-list-assignments.h"

#include "compiler/name-gen.h"

VertexPtr ConvertListAssignmentsPass::process_list_assignment(VertexAdaptor<op_list> list) {
  VertexPtr op_set_to_tmp_var;
  if (list->array()->type() != op_var) {        // list(...) = $var не трогаем, только list(...) = f()
    auto tmp_var = VertexAdaptor<op_var>::create();
    tmp_var->set_string(gen_unique_name("tmp_var"));
    tmp_var->extra_type = op_ex_var_superlocal_inplace;        // отвечает требованиям: инициализируется 1 раз и внутри set
    auto set_var = VertexAdaptor<op_set>::create(tmp_var, list->array());
    list->array() = tmp_var.clone();
    op_set_to_tmp_var = set_var;
  }

  VertexPtr result_seq;
  if (!op_set_to_tmp_var) {
    auto seq = VertexAdaptor<op_seq_rval>::create(list, list->array().as<op_var>().clone());
    result_seq = seq;
  } else {
    auto seq = VertexAdaptor<op_seq_rval>::create(op_set_to_tmp_var, list, list->array().as<op_var>().clone());
    result_seq = seq;
  }
  set_location(result_seq, list->location);
  return result_seq;
}
VertexPtr ConvertListAssignmentsPass::on_exit_vertex(VertexPtr root, LocalT *local) {
  if (root->type() == op_list) {
    local->need_recursion_flag = false;
    return process_list_assignment(root.as<op_list>());
  }

  return root;
}
