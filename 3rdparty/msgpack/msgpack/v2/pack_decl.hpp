//
// MessagePack for C++ serializing routine
//
// Copyright (C) 2008-2016 FURUHASHI Sadayuki and KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef MSGPACK_V2_PACK_DECL_HPP
#define MSGPACK_V2_PACK_DECL_HPP

#include "msgpack/v1/pack_decl.hpp"

namespace msgpack {

/// @cond
MSGPACK_API_VERSION_NAMESPACE(v2) {
/// @endcond

using v1::packer;

using v1::pack;


using v1::take8_8;

using v1::take8_16;

using v1::take8_32;

using v1::take8_64;

/// @cond
}  // MSGPACK_API_VERSION_NAMESPACE(v2)
/// @endcond

}  // namespace msgpack

#endif // MSGPACK_V2_PACK_DECL_HPP
