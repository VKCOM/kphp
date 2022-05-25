//
// MessagePack for C++ deserializing routine
//
// Copyright (C) 2018 KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef MSGPACK_V3_UNPACK_DECL_HPP
#define MSGPACK_V3_UNPACK_DECL_HPP

#include "msgpack/v2/unpack_decl.hpp"

namespace msgpack {

/// @cond
MSGPACK_API_VERSION_NAMESPACE(v3) {
/// @endcond

using v2::unpack_reference_func;

using v2::unpack_error;
using v2::parse_error;
using v2::insufficient_bytes;
using v2::size_overflow;
using v2::array_size_overflow;
using v2::map_size_overflow;
using v2::str_size_overflow;
using v2::bin_size_overflow;
using v2::ext_size_overflow;
using v2::depth_size_overflow;
using v2::unpack_limit;

namespace detail {

using v2::detail::unpack_user;

using v2::detail::unpack_uint8;
using v2::detail::unpack_uint16;
using v2::detail::unpack_uint32;
using v2::detail::unpack_uint64;

using v2::detail::unpack_int8;
using v2::detail::unpack_int16;
using v2::detail::unpack_int32;
using v2::detail::unpack_int64;

using v2::detail::unpack_float;
using v2::detail::unpack_double;

using v2::detail::unpack_nil;

using v2::detail::unpack_true;
using v2::detail::unpack_false;

using v2::detail::unpack_array;
using v2::detail::unpack_array_item;
using v2::detail::unpack_map;
using v2::detail::unpack_map_item;
using v2::detail::unpack_str;
using v2::detail::unpack_bin;
using v2::detail::unpack_ext;

using  v2::detail::unpack_stack;

using v2::detail::init_count;
using v2::detail::decr_count;
using v2::detail::incr_count;

using v2::detail::get_count;

using v2::detail::fix_tag;

using v2::detail::value;

using v2::detail::load;

} // detail


using v2::unpacked;

namespace detail {

using v2::detail::unpack_imp;

} // detail

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
    msgpack::unpack_reference_func f = MSGPACK_NULLPTR, void* user_data = MSGPACK_NULLPTR, msgpack::unpack_limit const& limit = unpack_limit());

/// @cond
}  // MSGPACK_API_VERSION_NAMESPACE(v3)
/// @endcond

}  // namespace msgpack

#endif // MSGPACK_V3_UNPACK_DECL_HPP
