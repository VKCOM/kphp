//
// MessagePack for C++ deserializing routine
//
// Copyright (C) 2016 KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef MSGPACK_V2_UNPACK_DECL_HPP
#define MSGPACK_V2_UNPACK_DECL_HPP

#include "msgpack/v1/unpack_decl.hpp"

namespace msgpack {

/// @cond
MSGPACK_API_VERSION_NAMESPACE(v2) {
/// @endcond

using v1::unpack_reference_func;

using v1::unpack_error;
using v1::parse_error;
using v1::insufficient_bytes;
using v1::size_overflow;
using v1::array_size_overflow;
using v1::map_size_overflow;
using v1::str_size_overflow;
using v1::bin_size_overflow;
using v1::ext_size_overflow;
using v1::depth_size_overflow;
using v1::unpack_limit;

namespace detail {

using v1::detail::fix_tag;

using v1::detail::value;

using v1::detail::load;

} // detail



namespace detail {

parse_return
unpack_imp(const char* data, std::size_t len, std::size_t& off,
           msgpack::zone& result_zone, msgpack::object& result, bool& referenced,
           unpack_reference_func f, void* user_data,
           unpack_limit const& limit);


} // detail

/// @cond
}  // MSGPACK_API_VERSION_NAMESPACE(v2)
/// @endcond

}  // namespace msgpack

#endif // MSGPACK_V2_UNPACK_DECL_HPP
