// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <forward_list>

#include "common/wrappers/iterator_range.h"
#include "common/mixin/not_copyable.h"
#include "runtime/runtime-types.h"

class KphpBacktrace : vk::not_copyable {
public:
  KphpBacktrace(void *const *raw_backtrace, int32_t size) noexcept;

  auto make_demangled_backtrace_range(bool full_trace = false) noexcept {
    return vk::make_transform_iterator_range(
      [this, full_trace](const char *symbol) { return make_line(symbol, full_trace); },
      symbols_begin_, symbols_end_
    );
  }

  ~KphpBacktrace() noexcept {
    clear();
  }

private:
  const char *make_line(const char *symbol, bool full_trace) noexcept;

  void clear() noexcept;

  char **symbols_begin_{nullptr};
  char **symbols_end_{nullptr};
  std::array<char, 1024> name_buffer_;
  std::array<char, 2048> trace_buffer_;

  friend void free_kphp_backtrace() noexcept;

  // used symbols are stored here in case we get an unexpected signal
  static std::forward_list<char **> last_used_symbols_;
};

array<string> f$kphp_backtrace(bool pretty = true) noexcept;

void parse_kphp_backtrace(char * buffer, size_t buffer_len, void * const * raw_backtrace, int backtrace_len);

void free_kphp_backtrace() noexcept;
