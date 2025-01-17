// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstddef>

#include <execinfo.h>
#include <array>

#include "runtime-common/core/utils/kphp-assert-core.h"

extern "C" void *__wrap_malloc([[maybe_unused]] size_t size) noexcept {
  std::array<void *, 64> b;
  int s = backtrace(b.data(), 64);
  backtrace_symbols_fd(b.data(), s, 2);

  php_critical_error("unexpected use of malloc");
}

extern "C" void __wrap_free([[maybe_unused]] void *ptr) noexcept {
  php_critical_error("unexpected use of free");
}

extern "C" void *__wrap_calloc([[maybe_unused]] size_t nmemb, [[maybe_unused]] size_t size) noexcept {
  php_critical_error("unexpected use of calloc");
}

extern "C" void *__wrap_realloc([[maybe_unused]] void *ptr, [[maybe_unused]] size_t size) noexcept {
  php_critical_error("unexpected use of realloc");
}
