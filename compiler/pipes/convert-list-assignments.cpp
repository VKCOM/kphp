#include "compiler/pipes/convert-list-assignments.h"

#include "compiler/name-gen.h"

VertexPtr ConvertListAssignmentsPass::process_list_assignment(VertexAdaptor<op_list> list) {
  // save right part to temporary variable in cases:
  //   list($x,) = call_fun();
  //   list($x,) = $x;
  // replace it with AST like this:
  //   list($x,) = ({ auto tmp_var = call_fun(); <op_list>; tmp_var; });
  auto same_var_on_lhs_and_rhs_sides = [list] {
    for (auto list_item : list->list()) {
      if (auto var_in_list = list_item.try_as<op_var>()) {
        if (var_in_list->str_val == list->array().as<op_var>()->str_val) {
          return true;
        }
      }
    }
    return false;
  };

  VertexAdaptor<op_seq_rval> result_seq;
  bool is_not_var_on_right_side = (list->array()->type() != op_var);
  if (is_not_var_on_right_side || same_var_on_lhs_and_rhs_sides()) {
    auto tmp_var = VertexAdaptor<op_var>::create();
    tmp_var->set_string(gen_unique_name("tmp_var"));
    tmp_var->extra_type = op_ex_var_superlocal_inplace;
    auto set_var = VertexAdaptor<op_set>::create(tmp_var, list->array());
    list->array() = tmp_var.clone();
    result_seq = VertexAdaptor<op_seq_rval>::create(set_var, list, list->array().as<op_var>().clone());
  } else {
    result_seq = VertexAdaptor<op_seq_rval>::create(list, list->array().as<op_var>().clone());
  }

  return result_seq.set_location(list);
}

VertexPtr ConvertListAssignmentsPass::on_exit_vertex(VertexPtr root) {
  if (root->type() == op_list) {
    return process_list_assignment(root.as<op_list>());
  }

  return root;
}
