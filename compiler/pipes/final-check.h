// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/var-data.h"
#include "compiler/function-pass.h"
#include "compiler/inferring/public.h"

class FinalCheckPass final : public FunctionPassBase {
private:
  void check_static_var_inited(VarPtr static_var);
  
public:

  std::string get_description() override {
    return "Final check";
  }

  bool check_function(FunctionPtr function) const override {
    return !function->is_extern();
  }

  void on_start() override;
  void on_function();

  VertexPtr on_enter_vertex(VertexPtr vertex) override;

  bool user_recursion(VertexPtr v) override;

  VertexPtr on_exit_vertex(VertexPtr vertex) override;

private:
  void check_op_func_call(VertexAdaptor<op_func_call> call);
  void check_lib_exported_function(FunctionPtr function);
  void check_eq3(VertexPtr lhs, VertexPtr rhs);
  void check_comparisons(VertexPtr lhs, VertexPtr rhs, Operation op);
  void raise_error_using_Unknown_type(VertexPtr v);

  static void check_magic_methods(FunctionPtr fun);
  static void check_magic_tostring_method(FunctionPtr fun);
  static void check_magic_clone_method(FunctionPtr fun);
  static void check_instanceof(VertexAdaptor<op_instanceof> instanceof_vertex);
  static void check_indexing(VertexPtr array, VertexPtr key) noexcept;
  static void check_array_literal(VertexAdaptor<op_array> vertex) noexcept;

  static void check_serialized_fields_hierarchy(ClassPtr klass);
};
