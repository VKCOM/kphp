// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/preprocess-break.h"

int PreprocessBreakPass::get_label_id(VertexAdaptor<meta_op_cycle> cycle, Operation op) {
  int &label_id = op == op_break ? cycle->break_label_id : cycle->continue_label_id;
  if (label_id == 0) {
    label_id = ++current_label_id;
  }
  return label_id;
}

VertexPtr PreprocessBreakPass::on_enter_vertex(VertexPtr root) {
  if (OpInfo::type(root->type()) == cycle_op) {
    cycles.push_back(root.as<meta_op_cycle>());
  }

  if (vk::any_of_equal(root->type(), op_break, op_continue)) {
    auto goto_op = root.as<meta_op_goto>();
    kphp_error_act(goto_op->level()->type() == op_int_const, "Break/continue parameter expected to be constant integer", return root);
    int val = atoi(goto_op->level()->get_string().c_str());
    kphp_error_act(1 <= val && val <= 10, "Break/continue parameter expected to be in [1;10] interval", return root);

    int cycles_n = static_cast<int>(cycles.size());
    kphp_error_act(val <= cycles_n, "Break/continue parameter is too big", return root);

    bool force_label = goto_op->type() == op_continue && val == 1 && !cycles.empty() && cycles.back()->type() == op_switch;
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
