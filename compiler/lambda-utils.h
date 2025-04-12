// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/threading/data-stream.h"
#include "compiler/vertex.h"

// see the .cpp file for detailed comments of how do lambdas work

void auto_capture_vars_from_body_in_arrow_lambda(FunctionPtr f_lambda);
void patch_lambda_function_add_uses_as_arguments(FunctionPtr f_lambda);
ClassPtr generate_lambda_class_wrapping_lambda_function(FunctionPtr f_lambda);
VertexPtr generate_lambda_class_construct_call_replacing_op_lambda(FunctionPtr f_lambda, VertexAdaptor<op_lambda> v_lambda);
InterfacePtr generate_interface_from_typed_callable(const TypeHint *type_hint);
void inherit_lambda_class_from_typed_callable(ClassPtr c_lambda, InterfacePtr typed_interface);
FunctionPtr generate_lambda_from_string_or_array(FunctionPtr current_function, const std::string &func_name, VertexPtr bound_this, VertexPtr v_location);
