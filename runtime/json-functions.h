// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/exception.h"
#include "runtime/kphp_core.h"

constexpr int64_t JSON_UNESCAPED_UNICODE = 1;
constexpr int64_t JSON_FORCE_OBJECT = 16;
constexpr int64_t JSON_PARTIAL_OUTPUT_ON_ERROR = 512;
constexpr int64_t JSON_AVAILABLE_OPTIONS = JSON_UNESCAPED_UNICODE | JSON_FORCE_OBJECT | JSON_PARTIAL_OUTPUT_ON_ERROR;

namespace impl_ {

class JsonEncoder {
public:
  JsonEncoder(int64_t options, bool simple_encode) noexcept;

  bool encode(const mixed &v) noexcept;

private:
  int64_t options_{0};
  bool simple_encode_{false};
};

} // namespace impl_

template<class T>
Optional<string> f$json_encode(const T &v, int64_t options = 0, bool simple_encode = false) noexcept {
  const bool has_unsupported_option = static_cast<bool>(options & ~JSON_AVAILABLE_OPTIONS);
  if (unlikely(has_unsupported_option)) {
    php_warning("Wrong parameter options = %" PRIi64 " in function json_encode", options);
    return false;
  }

  static_SB.clean();
  if (unlikely(!impl_::JsonEncoder(options, simple_encode).encode(v))) {
    return false;
  }
  return static_SB.str();
}

template<class T>
string f$vk_json_encode_safe(const T &v, bool simple_encode = true) noexcept {
  static_SB.clean();
  string_buffer::string_buffer_error_flag = STRING_BUFFER_ERROR_FLAG_ON;
  impl_::JsonEncoder(0, simple_encode).encode(v);
  if (unlikely(string_buffer::string_buffer_error_flag == STRING_BUFFER_ERROR_FLAG_FAILED)) {
    static_SB.clean();
    string_buffer::string_buffer_error_flag = STRING_BUFFER_ERROR_FLAG_OFF;
    THROW_EXCEPTION (new_Exception(string(__FILE__), __LINE__, string("json_encode buffer overflow", 27)));
    return string();
  }
  string_buffer::string_buffer_error_flag = STRING_BUFFER_ERROR_FLAG_OFF;
  return static_SB.str();
}

template<class T>
inline Optional<string> f$vk_json_encode(const T &v) noexcept {
  return f$json_encode(v, 0, true);
}

mixed f$json_decode(const string &v, bool assoc = false) noexcept;
