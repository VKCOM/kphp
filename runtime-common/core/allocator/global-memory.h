//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2026 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

namespace kphp::memory::global {

auto alloc(size_t size) noexcept -> void*;
auto alloc0(size_t size) noexcept -> void*;
auto realloc(void* mem, size_t new_size, size_t old_size) noexcept -> void*;
auto free(void* mem, size_t size) noexcept -> void;

} // namespace kphp::memory::global
