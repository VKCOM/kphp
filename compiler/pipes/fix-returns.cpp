#include "compiler/pipes/fix-returns.h"

#include "compiler/data/src-file.h"
#include "compiler/inferring/public.h"

VertexPtr FixReturnsPass::on_enter_vertex(VertexPtr root, LocalT *) {
  auto is_void_fun = [](FunctionPtr f) {
    return tinf::get_type(f, -1)->ptype() == tp_void;
  };
  auto is_void_expr = [](VertexPtr root) {
    return tinf::get_type(root)->ptype() == tp_void;
  };

  if (root->rl_type == val_r && is_void_expr(root)) {
    if (root->type() == op_func_call) {
      FunctionPtr fun = root.as<op_func_call>()->get_func_id();
      kphp_error(0, format("Using result of void function %s", fun->get_human_readable_name().c_str()));
    } else {
      kphp_error(0, "Using result of void expression");
    }
  }

  if (auto return_op = root.try_as<op_return>()) {
    if (is_void_fun(current_function) && return_op->has_expr() && is_void_expr(return_op->expr())) {
      std::vector<VertexPtr> seq;
      seq.push_back(return_op->expr());
      seq.back()->rl_type = val_none;
      seq.push_back(VertexAdaptor<op_return>::create());
      seq.back()->location = return_op->location;
      return VertexAdaptor<op_seq>::create(seq);
    }
    return return_op;
  }

  return root;
}
