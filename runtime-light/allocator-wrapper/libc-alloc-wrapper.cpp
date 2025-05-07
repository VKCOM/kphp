// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstddef>

#include "runtime-light/utils/logs.h"

extern "C" void* __wrap_malloc([[maybe_unused]] size_t size) noexcept {
  kphp::log::fatal("unexpected use of malloc");
}

extern "C" void __wrap_free([[maybe_unused]] void* ptr) noexcept {
  kphp::log::fatal("unexpected use of free");
}

extern "C" void* __wrap_calloc([[maybe_unused]] size_t nmemb, [[maybe_unused]] size_t size) noexcept {
  kphp::log::fatal("unexpected use of calloc");
}

extern "C" void* __wrap_realloc([[maybe_unused]] void* ptr, [[maybe_unused]] size_t size) noexcept {
  kphp::log::fatal("unexpected use of realloc");
}
