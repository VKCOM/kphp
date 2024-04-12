// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/allocator/allocator.h"
#include "runtime-light/core/kphp_core.h"

string_buffer::string_buffer(string::size_type buffer_len) noexcept:
  buffer_end(static_cast<char *>(get_platform_allocator()->alloc(buffer_len))),
  buffer_begin(buffer_end),
  buffer_len(buffer_len) {
}

string_buffer::~string_buffer() noexcept {
  get_platform_allocator()->free(buffer_begin);
}
