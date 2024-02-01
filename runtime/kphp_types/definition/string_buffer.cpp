// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt


#include "runtime/kphp_core.h"

string_buffer static_SB;
string_buffer static_SB_spare;

string::size_type string_buffer::MIN_BUFFER_LEN = 266175;
string::size_type string_buffer::MAX_BUFFER_LEN = (1 << 24);

int string_buffer::string_buffer_error_flag = 0;

string_buffer::string_buffer(string::size_type buffer_len) noexcept:
  buffer_end(static_cast<char *>(platform::allocate(buffer_len))),
  buffer_begin(buffer_end),
  buffer_len(buffer_len) {
}

string_buffer::~string_buffer() noexcept {
  platform::deallocate(buffer_begin, buffer_len);
}
