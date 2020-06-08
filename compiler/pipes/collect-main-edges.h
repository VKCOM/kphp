#pragma once

#include "compiler/function-pass.h"
#include "compiler/inferring/lvalue.h"
#include "compiler/inferring/rvalue.h"
#include "compiler/pipes/function-and-cfg.h"

class CollectMainEdgesPass final : public FunctionPassBase {
private:
  bool have_returns = false;

  static tinf::Node *node_from_rvalue(const RValue &rvalue);
  static void require_node(const RValue &rvalue);
  static void create_set(const LValue &lvalue, const RValue &rvalue);
  template<class RestrictionT>
  static void create_restriction(const RValue &lhs, const RValue &rhs);
  static void create_less(const RValue &lhs, const RValue &rhs);
  static void create_greater(const RValue &lhs, const RValue &rhs);
  static void create_non_void(const RValue &lhs);
  static void create_isset_check(const RValue &rvalue);
  static RValue as_set_value(VertexPtr v);


  template<class A, class B>
  static void create_set(const A &a, const B &b);
  template<class A, class B>
  static void create_less(const A &a, const B &b);
  template<class A, class B>
  static void create_greater(const A &a, const B &b);
  template<class A>
  static void create_non_void(const A &a);
  template<class A>
  static void require_node(const A &a);

  static void add_type_rule(VertexPtr v);
  static void add_type_help(VertexPtr v);
  static void on_func_param_callback(VertexAdaptor<op_func_call> call, int id);
  static void on_func_call(VertexAdaptor<op_func_call> call);
  void on_return(VertexAdaptor<op_return> v);
  static void on_foreach(VertexAdaptor<op_foreach> foreach_op);
  static void on_list(VertexAdaptor<op_list> list);
  static void on_throw(VertexAdaptor<op_throw> throw_op);
  static void on_try(VertexAdaptor<op_try> try_op);
  static void on_set_op(VertexPtr v);
  static void ifi_fix(VertexPtr v);
  static void on_function(FunctionPtr function);
  void on_class(ClassPtr klass);
  void on_var(VarPtr var);

  template<class CollectionT>
  void call_on_var(const CollectionT &collection);

public:

  using ExecuteType = FunctionAndCFG;

  string get_description() override {
    return "Collect main tinf edges";
  }

  void on_start() override;

  bool user_recursion(VertexPtr vertex) override;

  VertexPtr on_enter_vertex(VertexPtr v) override;

  void on_finish() override;
};
