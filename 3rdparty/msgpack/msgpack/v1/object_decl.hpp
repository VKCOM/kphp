//
// MessagePack for C++ static resolution routine
//
// Copyright (C) 2008-2014 FURUHASHI Sadayuki and KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef MSGPACK_V1_OBJECT_DECL_HPP
#define MSGPACK_V1_OBJECT_DECL_HPP

#include "msgpack/versioning.hpp"
#include "msgpack/pack.hpp"
#include "msgpack/zone.hpp"
#include "msgpack/adaptor/adaptor_base.hpp"

#include <cstring>
#include <stdexcept>
#include <typeinfo>
#include <limits>
#include <ostream>
#include <typeinfo>
#include <iomanip>

namespace msgpack {

/// @cond
MSGPACK_API_VERSION_NAMESPACE(v1) {
/// @endcond

/// The class holds object and zone
class object_handle;

namespace detail {

template <typename Stream, typename T>
struct packer_serializer;

} // namespace detail

/// @cond
}  // MSGPACK_API_VERSION_NAMESPACE(v1)
/// @endcond

}  // namespace msgpack

#endif // MSGPACK_V1_OBJECT_DECL_HPP
