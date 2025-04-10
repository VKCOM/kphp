// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/ffi/ffi_types.h"

void ffi_type_delete(const FFIType* type) {
  for (const auto* member : type->members) {
    ffi_type_delete(member);
  }
  delete type;
}
