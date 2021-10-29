// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

// Difficulties to make it a single pass instead of Begin+End separated by a sync pass:
//
// - Lambda capture vars assumptions calculation
// - Need to know FFI::scope() variable types to infer $scope->new result type correctly
// - Need to know instance() part type of the instance_prop exprs
//
// We solve these difficulties by replacing the main FFI inference roots
// in a separate pass (Begin) and then we can use assumptions on instance_prop->instance()
// and so on without problems.

class PreprocessFFIOperationsBegin final : public FunctionPassBase {
private:
  VertexPtr on_ffi_static_new(VertexAdaptor<op_func_call> call);
  VertexPtr on_ffi_scope_new(VertexAdaptor<op_func_call> call, ClassPtr scope_class);

  VertexPtr on_ffi_static_addr(VertexAdaptor<op_func_call> call);

  VertexPtr on_ffi_static_cast(VertexAdaptor<op_func_call> call);
  VertexPtr on_ffi_scope_cast(VertexAdaptor<op_func_call> call, ClassPtr scope_class);
  VertexPtr on_ffi_cast(VertexPtr call, VertexPtr type_string, VertexPtr arg, ClassPtr scope_class);

public:
  string get_description() override {
    return "Instrument the code that uses FFI operations";
  }

  bool check_function(FunctionPtr f) const override;

  VertexPtr on_exit_vertex(VertexPtr v) override;
};

class PreprocessFFIOperationsEnd final : public FunctionPassBase {
private:
  VertexPtr on_cdata_instance_prop(ClassPtr root_class, VertexAdaptor<op_instance_prop> root);
  VertexPtr on_scope_instance_prop(ClassPtr root_class, VertexAdaptor<op_instance_prop> root);

public:
  string get_description() override {
    return "Instrument the code that uses FFI operations";
  }

  bool check_function(FunctionPtr f) const override;

  VertexPtr on_enter_vertex(VertexPtr v) override;
  VertexPtr on_exit_vertex(VertexPtr v) override;
};
