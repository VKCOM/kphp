// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/wrappers/string_view.h"

namespace ffi {

// We can't use vk::string_view as a union member as it's not a trivial class.
//
// We could make it trivial by removing non-trivial constructor,
// but that would make a default vk::string_view value less useful and
// more error-prone to use.
//
// As a workaround, we pass this string_span wrapper around the parser.
struct string_span {
  const char *data_;
  size_t len_;

  string_span() = default;

  string_span(const char *data)
    : data_{data}
    , len_{std::strlen(data)} {}

  string_span(const char *data, const char *data_end)
    : data_{data}
    , len_{static_cast<size_t>(data_end - data)} {}

  std::string to_string() const {
    return std::string{data_, len_};
  }

  vk::string_view to_string_view() const {
    return vk::string_view{data_, len_};
  }
};

} // namespace ffi
