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

  static RValue as_set_value(VertexPtr v);

  template<class R>
  static void create_set(const LValue& lhs, const R& rhs);
  static void create_type_assign(const LValue& lhs, const TypeData* initial_type);
  static void create_type_assign_with_arg_ref_rule(const LValue& lhs, const TypeHint* type_hint, VertexPtr func_call);
  static void create_type_assign_with_restriction(const LValue& var_lhs, const TypeHint* type_hint);
  static void create_postponed_type_check(const RValue& restricted_value, const RValue& actual_value, const TypeHint* type_hint);
  static void create_non_void(const RValue& lhs);
  static void create_isset_check(const RValue& rvalue);
  static void create_edges_to_recalc_arg_ref(const TypeHint* type_hint, VertexPtr dependent_vertex, VertexPtr func_call);

  static void on_var_phpdoc(VertexAdaptor<op_phpdoc_var> var_op);
  static void on_passed_callback_to_builtin(VertexAdaptor<op_func_call> call, int param_i, VertexAdaptor<op_callback_of_builtin> v_callback);
  static void on_func_call_extern_modifying_arg_type(VertexAdaptor<op_func_call> call, FunctionPtr extern_function);
  static void on_func_call(VertexAdaptor<op_func_call> call);
  static void on_return(VertexAdaptor<op_return> v, FunctionPtr function);
  static void on_foreach(VertexAdaptor<op_foreach> foreach_op);
  static void on_switch(VertexAdaptor<op_switch> switch_op);
  static void on_list(VertexAdaptor<op_list> list);
  static void on_try(VertexAdaptor<op_try> try_op);
  static void on_set_op(VertexPtr v);
  static void ifi_fix(VertexPtr v);
  static void on_function(FunctionPtr function);
  static void on_class(ClassPtr klass);
  static void on_var(VarPtr var);
  static void search_for_nested_calls(VertexPtr v);

  template<class CollectionT>
  void call_on_var(const CollectionT& collection);

public:
  using ExecuteType = FunctionAndCFG;

  std::string get_description() override {
    return "Collect main tinf edges";
  }

  void on_start() override;

  VertexPtr on_enter_vertex(VertexPtr v) override;

  void on_finish() override;
};
