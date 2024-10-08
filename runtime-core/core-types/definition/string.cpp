// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-core/runtime-core.h"

// Don't move this destructor to the headers, it spoils addr2line traces
string::~string() noexcept {
  destroy();
}

string::string(double f) {
  constexpr uint32_t MAX_LEN = 4096;
  char result[MAX_LEN + 2];
  result[0] = '\0';
  result[1] = '\0';

  char *begin = result + 2;
  if (std::isnan(f)) {
    // to prevent printing `-NAN` by snprintf
    f = std::abs(f);
 }
  int len = snprintf(begin, MAX_LEN, "%.14G", f);
  if (static_cast<uint32_t>(len) < MAX_LEN) {
    if (static_cast<uint32_t>(begin[len - 1] - '5') < 5 && begin[len - 2] == '0' && begin[len - 3] == '-') {
      --len;
      begin[len - 1] = begin[len];
    }
    if (begin[1] == 'E') {
      result[0] = begin[0];
      result[1] = '.';
      result[2] = '0';
      begin = result;
      len += 2;
    } else if (begin[0] == '-' && begin[2] == 'E') {
      result[0] = begin[0];
      result[1] = begin[1];
      result[2] = '.';
      result[3] = '0';
      begin = result;
      len += 2;
    }
    php_assert (len <= STRLEN_FLOAT);
    p = create(begin, begin + len);
  } else {
    php_warning("Maximum length of float (%d) exceeded", MAX_LEN);
    p = string_cache::empty_string().ref_data();
  }
}

void string::set_size(size_type new_size) {
  if (inner()->is_shared()) {
    string_inner *r = string_inner::create(new_size, capacity());

    inner()->dispose();
    p = r->ref_data();
  } else if (new_size > capacity()) {
    p = inner()->reserve(new_size);
  }
  inner()->set_length_and_sharable(new_size);
}

static_assert(sizeof(string) == SIZEOF_STRING, "sizeof(string) at runtime doesn't match compile-time");
