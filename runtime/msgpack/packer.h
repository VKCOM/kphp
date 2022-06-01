//
// MessagePack for C++ serializing routine
//
// Copyright (C) 2008-2016 FURUHASHI Sadayuki and KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <cstdint>
#include <cstddef>

#include "common/mixin/not_copyable.h"

#include "runtime/msgpack/adaptor/adaptor_base.h"

namespace msgpack {

/// The class template that supports continuous packing.
/**
 * @tparam Stream  Any type that have a member function `Stream write(const char*, size_t s)`
 *
 */

template<typename Stream>
class packer : private vk::not_copyable {
public:
  /// Constructor
  /**
   * @param s Packing destination stream object.
   */
  explicit packer(Stream &s);

  /// Packing function template
  /**
   * @tparam T The type of packing object.
   *
   * @param v a packing object.
   *
   * @return The reference of `*this`.
   */
  template<typename T>
  void pack(const T &v) {
    adaptor::pack<T>{}(*this, v);
  }

  /// Packing uint8
  /**
   * The byte size of the packed data depends on `d`.
   * The packed type is positive fixnum or uint8.
   * The minimum byte size expression is used.
   * See https://github.com/msgpack/msgpack/blob/master/spec.md#formats-int
   *
   * @param d a packing object.
   *
   * @return The reference of `*this`.
   */
  packer<Stream> &pack_uint8(uint8_t d);

  /// Packing uint32
  /**
   * The byte size of the packed data depends on `d`.
   * The packed type is positive fixnum, uint8, uint16 or uint32.
   * The minimum byte size expression is used.
   * See https://github.com/msgpack/msgpack/blob/master/spec.md#formats-int
   *
   * @param d a packing object.
   *
   * @return The reference of `*this`.
   */
  packer<Stream> &pack_uint32(uint32_t d);

  /// Packing uint16
  /**
   * The byte size of the packed data depends on `d`.
   * The packed type is positive fixnum, uint8, uint16, uint32 or uint64.
   * The minimum byte size expression is used.
   * positive fixnum, uint8, uint16, or uint32 is used.
   * See https://github.com/msgpack/msgpack/blob/master/spec.md#formats-int
   *
   * @param d a packing object.
   *
   * @return The reference of `*this`.
   */
  packer<Stream> &pack_uint64(uint64_t d);

  /// Packing int32
  /**
   * The byte size of the packed data depends on `d`.
   * If `d` is zero or positive, the packed type is positive fixnum, uint8, uint16, or uint32,
   * else the packed type is negative fixnum, int8, int16, or int32.
   * The minimum byte size expression is used.
   * See https://github.com/msgpack/msgpack/blob/master/spec.md#formats-int
   *
   * @param d a packing object.
   *
   * @return The reference of `*this`.
   */
  packer<Stream> &pack_int32(int32_t d);

  /// Packing int32
  /**
   * The byte size of the packed data depends on `d`.
   * If `d` is zero or positive, the packed type is positive fixnum, uint8, uint16, uint32, or uint64,
   * else the packed type is negative fixnum, int8, int16, int32, or int64.
   * The minimum byte size expression is used.
   * See https://github.com/msgpack/msgpack/blob/master/spec.md#formats-int
   *
   * @param d a packing object.
   *
   * @return The reference of `*this`.
   */
  packer<Stream> &pack_int64(int64_t d);

  /// Packing float
  /**
   * The packed type is float32.
   * See https://github.com/msgpack/msgpack/blob/master/spec.md#formats-float
   *
   * @param d a packing object.
   *
   * @return The reference of `*this`.
   */
  packer<Stream> &pack_float(float d);

  /// Packing double
  /**
   * The packed type is float64.
   * See https://github.com/msgpack/msgpack/blob/master/spec.md#formats-float
   *
   * @param d a packing object.
   *
   * @return The reference of `*this`.
   */
  packer<Stream> &pack_double(double d);

  /// Packing nil
  /**
   * The packed type is nil.
   * See https://github.com/msgpack/msgpack/blob/master/spec.md#formats-nil
   *
   * @return The reference of `*this`.
   */
  packer<Stream> &pack_nil();

  /// Packing true
  /**
   * The packed type is bool, value is true.
   * See https://github.com/msgpack/msgpack/blob/master/spec.md#bool-format-family
   *
   * @return The reference of `*this`.
   */
  packer<Stream> &pack_true();

  /// Packing false
  /**
   * The packed type is bool, value is false.
   * See https://github.com/msgpack/msgpack/blob/master/spec.md#bool-format-family
   *
   * @return The reference of `*this`.
   */
  packer<Stream> &pack_false();

  /// Packing array header and size
  /**
   * The packed type is array header and array size.
   * You need to pack `n` msgpack objects following this header and size.
   * See https://github.com/msgpack/msgpack/blob/master/spec.md#array-format-family
   *
   * @param n The number of array elements (array size).
   *
   * @return The reference of `*this`.
   */
  packer<Stream> &pack_array(uint32_t n);

  /// Packing map header and size
  /**
   * The packed type is map header and map size.
   * You need to pack `n` pairs of msgpack objects following this header and size.
   * See https://github.com/msgpack/msgpack/blob/master/spec.md#map-format-family
   *
   * @param n The number of array elements (array size).
   *
   * @return The reference of `*this`.
   */
  packer<Stream> &pack_map(uint32_t n);

  /// Packing str header and length
  /**
   * The packed type is str header and length.
   * The minimum byte size length expression is used.
   * You need to call `pack_str_body(const char* b, uint32_t l)` after this function calling with the same `l` value.
   * See https://github.com/msgpack/msgpack/blob/master/spec.md#formats-str
   *
   * @param l The length of string.
   *
   * @return The reference of `*this`.
   */
  packer<Stream> &pack_str(uint32_t l);

  /// Packing str body
  /**
   * You need to call this function just after `pack_str(uint32_t l)` calling.
   * The value `l` should be the same as `pack_str(uint32_t l)` argument `l`.
   * See https://github.com/msgpack/msgpack/blob/master/spec.md#formats-str
   *
   * @param l The length of string.
   *
   * @return The reference of `*this`.
   */
  packer<Stream> &pack_str_body(const char *b, uint32_t l);

private:
  template<typename T>
  void pack_imp_uint8(T d);
  template<typename T>
  void pack_imp_uint32(T d);
  template<typename T>
  void pack_imp_uint64(T d);
  template<typename T>
  void pack_imp_int32(T d);
  template<typename T>
  void pack_imp_int64(T d);

  void append_buffer(const char *buf, size_t len) {
    m_stream.write(buf, len);
  }

  Stream &m_stream;
};

/// Pack the value as MessagePack format into the stream
/**
 * @tparam Stream Any type that have a member function `Stream write(const char*, size_t s)`
 * @tparam T Any type that is adapted to MessagePack
 * @param s Packing destination stream
 * @param v Packing value
 */
template<typename Stream, typename T>
void pack(Stream &s, const T &v) {
  packer<Stream>(s).pack(v);
}

} // namespace msgpack
