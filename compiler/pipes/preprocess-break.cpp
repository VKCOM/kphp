#include "compiler/pipes/preprocess-break.h"

int PreprocessBreakPass::get_label_id(VertexAdaptor<meta_op_cycle> cycle, Operation op) {
  int *val = nullptr;
  if (op == op_break) {
    val = &cycle->break_label_id;
  } else if (op == op_continue) {
    val = &cycle->continue_label_id;
  } else {
    assert (0);
  }
  if (*val == 0) {
    *val = ++current_label_id;
  }
  return *val;
}
VertexPtr PreprocessBreakPass::on_enter_vertex(VertexPtr root) {
  if (OpInfo::type(root->type()) == cycle_op) {
    cycles.push_back(root.as<meta_op_cycle>());
  }

  if (root->type() == op_break || root->type() == op_continue) {
    int val;
    auto goto_op = root.as<meta_op_goto>();
    kphp_error_act (
      goto_op->level()->type() == op_int_const,
      "Break/continue parameter expected to be constant integer",
      return root
    );
    val = atoi(goto_op->level()->get_string().c_str());
    kphp_error_act (
      1 <= val && val <= 10,
      "Break/continue parameter expected to be in [1;10] interval",
      return root
    );

    bool force_label = false;
    if (goto_op->type() == op_continue && val == 1 && !cycles.empty()
        && cycles.back()->type() == op_switch) {
      force_label = true;
    }


    int cycles_n = (int)cycles.size();
    kphp_error_act (
      val <= cycles_n,
      "Break/continue parameter is too big",
      return root
    );
    if (val != 1 || force_label) {
      goto_op->int_val = get_label_id(cycles[cycles_n - val], root->type());
    }
  }

  return root;
}
VertexPtr PreprocessBreakPass::on_exit_vertex(VertexPtr root) {
  if (OpInfo::type(root->type()) == cycle_op) {
    cycles.pop_back();
  }
  return root;
}
