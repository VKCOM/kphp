#pragma once

#include "compiler/function-pass.h"
#include "compiler/pipes/function-and-cfg.h"

struct RValue;
struct LValue;

class CollectMainEdgesPass : public FunctionPassBase {
private:
  tinf::Node *node_from_rvalue(const RValue &rvalue);
  void require_node(const RValue &rvalue);
  void create_set(const LValue &lvalue, const RValue &rvalue);
  void create_less(const RValue &lhs, const RValue &rhs);
  void create_isset_check(const RValue &rvalue);
  RValue as_set_value(VertexPtr v);


  template<class A, class B>
  void create_set(const A &a, const B &b);
  template<class A, class B>
  void create_less(const A &a, const B &b);
  template<class A>
  void require_node(const A &a);

  void add_type_rule(VertexPtr v);
  void add_type_help(VertexPtr v);
  void on_func_param_callback(VertexAdaptor<op_func_call> call, int id);
  void on_func_call(VertexAdaptor<op_func_call> call);
  void on_constructor_call(VertexAdaptor<op_constructor_call> call);
  void on_return(VertexAdaptor<op_return> v);
  void on_foreach(VertexAdaptor<op_foreach> foreach_op);
  void on_list(VertexAdaptor<op_list> list);
  void on_fork(VertexAdaptor<op_fork> fork);
  void on_throw(VertexAdaptor<op_throw> throw_op);
  void on_try(VertexAdaptor<op_try> try_op);
  void on_set_op(VertexPtr v);
  void ifi_fix(VertexPtr v);
  void on_function(FunctionPtr function);
  void on_var(VarPtr var);

  template<class CollectionT>
  void call_on_var(const CollectionT &collection);

public:

  using ExecuteType = FunctionAndCFG;

  string get_description() {
    return "Collect main tinf edges";
  }

  bool on_start(FunctionPtr function);

  VertexPtr on_enter_vertex(VertexPtr v, LocalT *local __attribute__((unused)));


  nullptr_t on_finish();
};
