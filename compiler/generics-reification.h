// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/vertex.h"

class GenericsDeclarationMixin;

void apply_instantiationTs_from_php_comment(FunctionPtr generic_function, VertexAdaptor<op_func_call> call, const GenericsInstantiationPhpComment *commentTs);
void reify_function_genericTs_on_generic_func_call(FunctionPtr current_function, FunctionPtr generic_function, VertexAdaptor<op_func_call> call,
                                                   bool old_syntax);
void check_reifiedTs_for_generic_func_call(const GenericsDeclarationMixin *genericTs, VertexAdaptor<op_func_call> call, bool old_syntax);
FunctionPtr convert_variadic_generic_function_accepting_N_args(FunctionPtr generic_function, int n_variadic);
