// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class InstantiateFFIOperationsPass final : public FunctionPassBase {
private:
  VertexPtr on_ffi_static_new(VertexAdaptor<op_func_call> call);
  VertexPtr on_ffi_scope_new(VertexAdaptor<op_func_call> call, ClassPtr scope_class);

  VertexPtr on_ffi_static_addr(VertexAdaptor<op_func_call> call);
  VertexPtr on_ffi_static_is_null(VertexAdaptor<op_func_call> call);

  VertexPtr on_ffi_static_cast(VertexAdaptor<op_func_call> call);
  VertexPtr on_ffi_scope_cast(VertexAdaptor<op_func_call> call, ClassPtr scope_class);

  VertexPtr on_ffi_array_get(VertexAdaptor<op_func_call> call);
  VertexPtr on_ffi_array_set(VertexAdaptor<op_func_call> call);

  VertexPtr on_ffi_scope_call_custom(VertexAdaptor<op_func_call> call);

  VertexPtr on_cdata_instance_prop(ClassPtr root_class, VertexAdaptor<op_instance_prop> root);
  VertexPtr on_scope_instance_prop(ClassPtr scope_class, VertexAdaptor<op_instance_prop> root);

public:
  std::string get_description() override {
    return "Instrument the code that uses FFI operations";
  }

  VertexPtr on_enter_vertex(VertexPtr v) override;
  VertexPtr on_exit_vertex(VertexPtr v) override;

  // calc @return of FFI::new() and others, that can't be expressed in functions.txt
  // they are used in assumptions also
  static const TypeHint *infer_from_ffi_static_new(VertexAdaptor<op_func_call> call);
  static const TypeHint *infer_from_ffi_scope_new(VertexAdaptor<op_func_call> call, ClassPtr scope_class);
  static const TypeHint *infer_from_ffi_static_cast(FunctionPtr f, VertexAdaptor<op_func_call> call);
  static const TypeHint *infer_from_ffi_scope_cast(FunctionPtr f, VertexAdaptor<op_func_call> call, ClassPtr scope_class);
  static const TypeHint *infer_from_ffi_array_get(FunctionPtr f, VertexAdaptor<op_func_call> call);
};
