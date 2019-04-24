#include "compiler/pipes/fix-returns.h"

#include "compiler/data/src-file.h"
#include "compiler/inferring/public.h"

namespace {
void warn_on_mixing(FunctionPtr fun) {
  if (fun->type != FunctionData::func_global && fun->type != FunctionData::func_switch) {
    kphp_typed_warning("return", "Mixing void and not void returns in one function");
  }
}
} // namespace

VertexPtr FixReturnsPass::on_enter_vertex(VertexPtr root, LocalT *) {
  auto is_void_fun = [](FunctionPtr f) {
    return tinf::get_type(f, -1)->ptype() == tp_void;
  };
  if (root->type() == op_func_call) {
    FunctionPtr fun = root.as<op_func_call>()->get_func_id();
    if (is_void_fun(fun) && root->rl_type == val_r) {
      kphp_error(0, format("Using result of void function %s\n", fun->get_human_readable_name().c_str()));
    }
    return root;
  }

  if (root->type() == op_return) {
    auto return_op = root.as<op_return>();
    bool is_current_function_void = is_void_fun(current_function);
    if (!return_op->has_expr()) {
      if (!is_current_function_void) {
        warn_on_mixing(current_function);
        auto new_return_op = VertexAdaptor<op_return>::create(VertexAdaptor<op_null>::create());
        new_return_op->location = return_op->location;
        return new_return_op;
      }
      return return_op;
    }

    if (tinf::get_type(return_op->expr())->ptype() == tp_void) {
      std::vector<VertexPtr> seq;
      seq.push_back(return_op->expr());
      seq.back()->rl_type = val_none;
      // hack!
      if (seq.back()->type() == op_require) {
        VertexAdaptor<op_require> require = seq.back().as<op_require>();
        kphp_assert(require->size() == 1 && require->back()->type() == op_func_call);
        require->back()->rl_type = val_none;
      }
      if (is_current_function_void) {
        seq.push_back(VertexAdaptor<op_return>::create());
      } else {
        warn_on_mixing(current_function);
        seq.push_back(VertexAdaptor<op_return>::create(VertexAdaptor<op_null>::create()));
      }
      seq.back()->location = return_op->location;
      return VertexAdaptor<op_seq>::create(seq);
    }
    return return_op;
  }

  return root;
}
