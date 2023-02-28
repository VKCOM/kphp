// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/vertex.h"

// maybe, replace f(...) with f'(...); see comments in cpp file
VertexPtr maybe_replace_extern_func_call(FunctionPtr current_function, VertexAdaptor<op_func_call> call);
