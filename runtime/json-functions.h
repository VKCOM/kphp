// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/exception.h"
#include "runtime/kphp_core.h"

constexpr int64_t JSON_UNESCAPED_UNICODE = 1;
constexpr int64_t JSON_FORCE_OBJECT = 16;
constexpr int64_t JSON_PRETTY_PRINT = 128; // TODO: add actual support
constexpr int64_t JSON_PARTIAL_OUTPUT_ON_ERROR = 512;
constexpr int64_t JSON_AVAILABLE_OPTIONS = JSON_UNESCAPED_UNICODE | JSON_FORCE_OBJECT | JSON_PARTIAL_OUTPUT_ON_ERROR;

namespace impl_ {

class JsonEncoder : vk::not_copyable {
public:
  JsonEncoder(int64_t options, bool simple_encode) noexcept;

  bool encode(bool b) const noexcept;
  bool encode(int64_t i) const noexcept;
  bool encode(double d) const noexcept;
  bool encode(const string &s) const noexcept;
  bool encode(const mixed &v) const noexcept;

  template<class T>
  bool encode(const array<T> &arr) const noexcept;

  template<class T>
  bool encode(const Optional<T> &opt) const noexcept;

private:
  bool encode_null() const noexcept;

  const int64_t options_{0};
  const bool simple_encode_{false};
};

template<class T>
bool JsonEncoder::encode(const array<T> &arr) const noexcept {
  bool is_vector = arr.is_vector();
  const bool force_object = static_cast<bool>(JSON_FORCE_OBJECT & options_);
  if (!force_object && !is_vector && arr.is_pseudo_vector()) {
    if (arr.get_next_key() == arr.count()) {
      is_vector = true;
    } else {
      php_warning("Corner case in json conversion, [] could be easy transformed to {}");
    }
  }
  is_vector &= !force_object;

  static_SB << "{["[is_vector];

  bool first = true;
  for (auto p : arr) {
    if (!first) {
      static_SB << ',';
    }
    first = false;

    if (!is_vector) {
      const auto key = p.get_key();
      if (array<T>::is_int_key(key)) {
        static_SB << '"' << key.to_int() << '"';
      } else {
        if (!encode(key)) {
          if (!(options_ & JSON_PARTIAL_OUTPUT_ON_ERROR)) {
            return false;
          }
        }
      }
      static_SB << ':';
    }

    if (!encode(p.get_value())) {
      if (!(options_ & JSON_PARTIAL_OUTPUT_ON_ERROR)) {
        return false;
      }
    }
  }

  static_SB << "}]"[is_vector];
  return true;
}

template<class T>
bool JsonEncoder::encode(const Optional<T> &opt) const noexcept {
  switch (opt.value_state()) {
    case OptionalState::has_value:
      return encode(opt.val());
    case OptionalState::false_value:
      return encode(false);
    case OptionalState::null_value:
      return encode_null();
  }
  __builtin_unreachable();
}

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
