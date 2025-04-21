// Compiler for PHP (aka KPHP)
// msgpack (c) https://github.com/msgpack/msgpack-c/tree/cpp_master (copied as third-party and slightly modified)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <type_traits>

#include "runtime-common/stdlib/msgpack/adaptor_base.h"

namespace vk::msgpack {
enum class stored_type {
  NIL = 0x00,
  BOOLEAN = 0x01,
  POSITIVE_INTEGER = 0x02,
  NEGATIVE_INTEGER = 0x03,
  FLOAT32 = 0x0a,
  FLOAT64 = 0x04,
  FLOAT = 0x04,
  STR = 0x05,
  ARRAY = 0x06,
  MAP = 0x07
};

struct object;
struct object_kv;

struct object_array {
  uint32_t size;
  object* ptr;
};

struct object_map {
  uint32_t size;
  object_kv* ptr;
};

struct object_str {
  uint32_t size;
  const char* ptr;
};

/// Object class that corresponding to MessagePack format object
/**
 * See https://github.com/msgpack/msgpack-c/wiki/v1_1_cpp_object
 */
struct object {
  union union_type {
    bool boolean;
    uint64_t u64;
    int64_t i64;
    double f64;
    object_array array;
    object_map map;
    object_str str;
  };

  stored_type type{stored_type::NIL};
  union_type via{};

  /// Get value as T
  /**
   * If the object can't be converted to T, type_error would be thrown.
   * @tparam T The type you want to get.
   * @return The converted object.
   */
  template<typename T>
  T as() const {
    // Without initializationg we have maybe-uninitialized warning.
    // Assume that default constructor is cheap.
    T v{};
    convert(v);
    return v;
  }

  /// Convert the object
  /**
   * If the object can't be converted to T, type_error would be thrown.
   * @tparam T The type of v.
   * @param v The value you want to get. `v` is output parameter. `v` is overwritten by converted value from the object.
   * @return The reference of `v`.
   */
  template<typename T>
  std::enable_if_t<!std::is_array_v<T> && !std::is_pointer_v<T>, T&> convert(T& v) const {
    adaptor::convert<T>{}(*this, v);
    return v;
  }
};

struct object_kv {
  object key;
  object val;
};

} // namespace vk::msgpack
