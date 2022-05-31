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

#include "runtime/msgpack/object.h"

namespace msgpack {

#ifndef MSGPACK_EMBED_STACK_SIZE
#define MSGPACK_EMBED_STACK_SIZE 32
#endif

/// Unpack msgpack::object from a buffer.
/**
 * @param data The pointer to the buffer.
 * @param len The length of the buffer.
 * @param off The offset position of the buffer. It is read and overwritten.
 *
 * @return object_handle that contains unpacked data.
 *
 */
msgpack::object_handle unpack(const char* data, std::size_t len, std::size_t& off);

}  // namespace msgpack
