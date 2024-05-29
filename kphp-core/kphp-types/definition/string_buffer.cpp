// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "kphp-core/kphp_core.h"

string_buffer::string_buffer(string::size_type buffer_len) noexcept:
  buffer_end(static_cast<char *>(KphpCoreContext::current().allocator.alloc_global_memory(buffer_len))),
  buffer_begin(buffer_end),
  buffer_len(buffer_len) {
}

string_buffer::~string_buffer() noexcept {
  KphpCoreContext::current().allocator.free_global_memory(buffer_begin, buffer_len);
}
