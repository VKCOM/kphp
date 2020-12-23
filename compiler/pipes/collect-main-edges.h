// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"
#include "compiler/inferring/lvalue.h"
#include "compiler/inferring/rvalue.h"
#include "compiler/pipes/function-and-cfg.h"

class CollectMainEdgesPass final : public FunctionPassBase {
private:
  bool have_returns = false;

  static tinf::Node *node_from_rvalue(const RValue &rvalue);
  static RValue as_set_value(VertexPtr v);


  template<class R>
  static void create_set(const LValue &lhs, const R &rhs);
  static void create_type_initial_assign(const LValue &lhs, const TypeData *initial_type);
  static void create_type_assign_with_restriction(const LValue &var_lhs, const TypeData *assigned_and_expected_type);
  static void create_postponed_type_check(const RValue &restricted_value, const RValue &actual_value, const TypeData *expected_type);
  static void create_non_void(const RValue &lhs);
  static void create_isset_check(const RValue &rvalue);

  static void add_type_rule(VertexAdaptor<op_var> var_op);
  static void on_func_call_param_callback(VertexAdaptor<op_func_call> call, int param_i, FunctionPtr provided_callback);
  static void on_func_call_extern_modifying_arg_type(VertexAdaptor<op_func_call> call, FunctionPtr extern_function);
  static void on_func_call(VertexAdaptor<op_func_call> call);
  void on_return(VertexAdaptor<op_return> v);
  static void on_foreach(VertexAdaptor<op_foreach> foreach_op);
  static void on_list(VertexAdaptor<op_list> list);
  static void on_try(VertexAdaptor<op_try> try_op);
  static void on_set_op(VertexPtr v);
  static void ifi_fix(VertexPtr v);
  static void on_function(FunctionPtr function);
  static void on_class(ClassPtr klass);
  static void on_var(VarPtr var);

  template<class CollectionT>
  void call_on_var(const CollectionT &collection);

public:

  using ExecuteType = FunctionAndCFG;

  string get_description() override {
    return "Collect main tinf edges";
  }

  void on_start() override;

  VertexPtr on_enter_vertex(VertexPtr v) override;

  void on_finish() override;
};
