//
// MessagePack for C++ static resolution routine
//
// Copyright (C) 2008-2014 FURUHASHI Sadayuki and KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

namespace msgpack {
namespace type {
enum object_type {
  NIL                 = 0x00,
  BOOLEAN             = 0x01,
  POSITIVE_INTEGER    = 0x02,
  NEGATIVE_INTEGER    = 0x03,
  FLOAT32             = 0x0a,
  FLOAT64             = 0x04,
  FLOAT               = 0x04,
  STR                 = 0x05,
  ARRAY               = 0x06,
  MAP                 = 0x07,
  BIN                 = 0x08,
  EXT                 = 0x09
};
} // namespace type


struct object;
struct object_kv;

struct object_array;
struct object_map;
struct object_str;
struct object_bin;
struct object_ext;


class type_error;

struct object_array {
    uint32_t size;
    msgpack::object* ptr;
};

struct object_map {
    uint32_t size;
    msgpack::object_kv* ptr;
};

struct object_str {
    uint32_t size;
    const char* ptr;
};

struct object_bin {
    uint32_t size;
    const char* ptr;
};

struct object_ext {
    int8_t type() const { return ptr[0]; }
    const char* data() const { return &ptr[1]; }
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
        int64_t  i64;
        double   f64;
        msgpack::object_array array;
        msgpack::object_map map;
        msgpack::object_str str;
        msgpack::object_bin bin;
        msgpack::object_ext ext;
    };

    msgpack::type::object_type type{type::NIL};
    union_type via;

    /// Get value as T
    /**
     * If the object can't be converted to T, msgpack::type_error would be thrown.
     * @tparam T The type you want to get.
     * @return The converted object.
     */
    template <typename T>
    T as() const;

    /// Convert the object
    /**
     * If the object can't be converted to T, msgpack::type_error would be thrown.
     * @tparam T The type of v.
     * @param v The value you want to get. `v` is output parameter. `v` is overwritten by converted value from the object.
     * @return The reference of `v`.
     */
    template <typename T>
    std::enable_if_t<!std::is_array_v<T> && !std::is_pointer_v<T>, T&>
    convert(T& v) const;
};

class type_error : public std::bad_cast { };

} // namespace msgpack
