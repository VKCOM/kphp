// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/core/runtime-core.h"

namespace {

struct test_runtime_context final {
  RuntimeContext context;

  test_runtime_context() noexcept {
    // Match runtime-light's string-buffer limits without component bindings.
    context.sb_lib_context.MIN_BUFFER_LEN = 1024;
    context.sb_lib_context.MAX_BUFFER_LEN = (1 << 24);
  }
};

} // namespace

auto RuntimeContext::get() noexcept -> RuntimeContext& {
  // Construct on first use, after the test allocator is available.
  static test_runtime_context state{};
  return state.context;
}
