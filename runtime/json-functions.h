// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/exception.h"
#include "runtime/kphp_core.h"

#include <array>

constexpr int64_t JSON_UNESCAPED_UNICODE = 1;
constexpr int64_t JSON_FORCE_OBJECT = 16;
constexpr int64_t JSON_PRETTY_PRINT = 128;
constexpr int64_t JSON_PARTIAL_OUTPUT_ON_ERROR = 512;
constexpr int64_t JSON_PRESERVE_ZERO_FRACTION = 1024;

constexpr int64_t JSON_AVAILABLE_OPTIONS = JSON_UNESCAPED_UNICODE | JSON_FORCE_OBJECT | JSON_PARTIAL_OUTPUT_ON_ERROR | JSON_PRESERVE_ZERO_FRACTION;
constexpr int64_t JSON_AVAILABLE_FLAGS_TYPED = JSON_PRETTY_PRINT;

struct JsonPath {
  constexpr static int MAX_DEPTH = 8;

  std::array<const char*, MAX_DEPTH> arr;
  unsigned depth = 0;

  void enter(const char *key) noexcept {
    if (depth < arr.size()) {
      arr[depth] = key;
    }
    depth++;
  }

  void leave() noexcept {
    depth--;
  }

  string to_string() const;
};

namespace impl_ {
// note: this class in runtime is used for non-typed json implementation: for json_encode() and json_decode()
// for classes, e.g. `JsonEncoder::encode(new A)`, see json-writer.h and from/to visitors
// todo somewhen, unify this JsonEncoder and JsonWriter, and support JSON_PRETTY_PRINT then
class JsonEncoder : vk::not_copyable {
public:
  JsonEncoder(int64_t options, bool simple_encode, const char *json_obj_magic_key = nullptr) noexcept;

  bool encode(bool b) noexcept;
  bool encode(int64_t i) noexcept;
  bool encode(const string &s) noexcept;
  bool encode(double d) noexcept;
  bool encode(const mixed &v) noexcept;

  template<class T>
  bool encode(const array<T> &arr) noexcept;

  template<class T>
  bool encode(const Optional<T> &opt) noexcept;

private:
  bool encode_null() const noexcept;

  JsonPath json_path_;
  const int64_t options_{0};
  const bool simple_encode_{false};
  const char *json_obj_magic_key_{nullptr};
};

template<class T>
bool JsonEncoder::encode(const array<T> &arr) noexcept {
  bool is_vector = arr.is_vector();
  const bool force_object = static_cast<bool>(JSON_FORCE_OBJECT & options_);
  if (!force_object && !is_vector && arr.is_pseudo_vector()) {
    if (arr.get_next_key() == arr.count()) {
      is_vector = true;
    } else {
      php_warning("%s: Corner case in json conversion, [] could be easy transformed to {}", json_path_.to_string().c_str());
    }
  }
  is_vector &= !force_object;

  static_SB << "{["[is_vector];

  if (is_vector) {
    int i = 0;
    json_path_.enter(nullptr); // similar key for all entries
    for (auto p : arr) {
      if (i != 0) {
        static_SB << ',';
      }
      if (!encode(p.get_value())) {
        if (!(options_ & JSON_PARTIAL_OUTPUT_ON_ERROR)) {
          return false;
        }
      }
      i++;
    }
    json_path_.leave();
  } else {
    bool is_first = true;
    for (auto p : arr) {
      if (!is_first) {
        static_SB << ',';
      }
      is_first = false;
      const char *next_key = nullptr;
      const auto key = p.get_key();
      if (array<T>::is_int_key(key)) {
        auto int_key = key.to_int();
        next_key = nullptr;
        static_SB << '"' << int_key << '"';
      } else {
        const string &str_key = key.as_string();
        // skip service key intended only for distinguish empty json object with empty json array
        if (json_obj_magic_key_ && !strcmp(json_obj_magic_key_, str_key.c_str())) {
          continue;
        }
        next_key = str_key.c_str();
        if (!encode(str_key)) {
          if (!(options_ & JSON_PARTIAL_OUTPUT_ON_ERROR)) {
            return false;
          }
        }
      }
      static_SB << ':';
      json_path_.enter(next_key);
      if (!encode(p.get_value())) {
        if (!(options_ & JSON_PARTIAL_OUTPUT_ON_ERROR)) {
          return false;
        }
      }
      json_path_.leave();
    }
  }

  static_SB << "}]"[is_vector];
  return true;
}

template<class T>
bool JsonEncoder::encode(const Optional<T> &opt) noexcept {
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
    return {};
  }
  string_buffer::string_buffer_error_flag = STRING_BUFFER_ERROR_FLAG_OFF;
  return static_SB.str();
}

template<class T>
inline Optional<string> f$vk_json_encode(const T &v) noexcept {
  return f$json_encode(v, 0, true);
}

std::pair<mixed, bool> json_decode(const string &v, const char *json_obj_magic_key = nullptr) noexcept;
mixed f$json_decode(const string &v, bool assoc = false) noexcept;
