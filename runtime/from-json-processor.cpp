// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/from-json-processor.h"

string JsonEncoderError::msg;

string f$JsonEncoder$$getLastError() noexcept {
  return JsonEncoderError::msg;
}
