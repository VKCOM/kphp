//
// MessagePack for C++ serializing routine
//
// Copyright (C) 2008-2016 FURUHASHI Sadayuki and KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef MSGPACK_V1_PACK_DECL_HPP
#define MSGPACK_V1_PACK_DECL_HPP

#include "msgpack/versioning.hpp"
#include "msgpack/sysdep.h"

namespace msgpack {

/// @cond
MSGPACK_API_VERSION_NAMESPACE(v3) {
/// @endcond

/// The class template that supports continuous packing.
/**
 * @tparam Stream  Any type that have a member function `Stream write(const char*, size_t s)`
 *
 */
template <typename Stream>
class packer;

/// Pack the value as MessagePack format into the stream
/**
 * This function template is left for compatibility.
 * Use `void pack(Stream& s, const T& v)` instead of the function template.
 *
 * @tparam Stream Any type that have a member function `Stream write(const char*, size_t s)`
 * @tparam T Any type that is adapted to MessagePack
 * @param s The pointer to packing destination stream
 * @param v Packing value
 */
template <typename Stream, typename T>
void pack(Stream* s, const T& v);

/// Pack the value as MessagePack format into the stream
/**
 * @tparam Stream Any type that have a member function `Stream write(const char*, size_t s)`
 * @tparam T Any type that is adapted to MessagePack
 * @param s Packing destination stream
 * @param v Packing value
 */
template <typename Stream, typename T>
void pack(Stream& s, const T& v);


/// @cond
}  // MSGPACK_API_VERSION_NAMESPACE(v1)
/// @endcond

}  // namespace msgpack

#endif // MSGPACK_V1_PACK_DECL_HPP
