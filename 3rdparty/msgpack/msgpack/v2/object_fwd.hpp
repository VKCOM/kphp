//
// MessagePack for C++ static resolution routine
//
// Copyright (C) 2008-2016 FURUHASHI Sadayuki and KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MSGPACK_V2_OBJECT_FWD_HPP
#define MSGPACK_V2_OBJECT_FWD_HPP

#include "msgpack/v2/object_fwd_decl.hpp"
#include "msgpack/object_fwd.hpp"

namespace msgpack {

/// @cond
MSGPACK_API_VERSION_NAMESPACE(v2) {
/// @endcond

struct object : v1::object {
    object() {}
    object(v1::object const& o):v1::object(o) {}
};

/// @cond
} // MSGPACK_API_VERSION_NAMESPACE(v2)
/// @endcond

} // namespace msgpack

#endif // MSGPACK_V2_OBJECT_FWD_HPP
