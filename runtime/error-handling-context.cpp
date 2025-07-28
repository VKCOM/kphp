// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/diagnostics/error-handling-context.h"

namespace {
ErrorHandlingContext error_handling_context{};
}

ErrorHandlingContext& ErrorHandlingContext::get() noexcept {
  return error_handling_context;
}
