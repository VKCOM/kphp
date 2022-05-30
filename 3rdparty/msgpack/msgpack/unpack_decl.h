//
// MessagePack for C++ deserializing routine
//
// Copyright (C) 2016 KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "msgpack/object.h"

namespace msgpack {

#ifndef MSGPACK_EMBED_STACK_SIZE
#define MSGPACK_EMBED_STACK_SIZE 32
#endif


class unpack_limit {
public:
  unpack_limit(
    std::size_t array = 0xffffffff,
    std::size_t map = 0xffffffff,
    std::size_t str = 0xffffffff,
    std::size_t bin = 0xffffffff,
    std::size_t ext = 0xffffffff,
    std::size_t depth = 0xffffffff)
    :array_(array),
    map_(map),
    str_(str),
    bin_(bin),
    ext_(ext),
    depth_(depth) {}
  std::size_t array() const { return array_; }
  std::size_t map() const { return map_; }
  std::size_t str() const { return str_; }
  std::size_t bin() const { return bin_; }
  std::size_t ext() const { return ext_; }
  std::size_t depth() const { return depth_; }

private:
  std::size_t array_;
  std::size_t map_;
  std::size_t str_;
  std::size_t bin_;
  std::size_t ext_;
  std::size_t depth_;
};

/// Unpack msgpack::object from a buffer.
/**
 * @param data The pointer to the buffer.
 * @param len The length of the buffer.
 * @param off The offset position of the buffer. It is read and overwritten.
 * @param f A judging function that msgpack::object refer to the buffer.
 * @param user_data This parameter is passed to f.
 * @param limit The size limit information of msgpack::object.
 *
 * @return object_handle that contains unpacked data.
 *
 */
msgpack::object_handle unpack(
  const char* data, std::size_t len, std::size_t& off,
  msgpack::unpack_limit const& limit = unpack_limit());

}  // namespace msgpack
