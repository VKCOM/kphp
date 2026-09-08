// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/function-pass.h"

template<class DataT>
class DataStream;

// RegisterFFIScopesF pass handles FFI class static method calls like FFI::cdef()
// and FFI::load() - it parses FFI strings and creates scope classes.
class RegisterFFIScopesF {
public:
  void execute(FunctionPtr f, DataStream<FunctionPtr>& os);
};
