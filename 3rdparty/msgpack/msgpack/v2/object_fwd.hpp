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
    /// Construct object from T
    /**
     * If `v` is the type that is corresponding to MessegePack format str, bin, ext, array, or map,
     * you need to call `object(const T& v, msgpack::zone& z)` instead of this constructor.
     *
     * @tparam T The type of `v`.
     * @param v The value you want to convert.
     */
    template <typename T>
    explicit object(const T& v) {
        *this << v;
    }

    /// Construct object from T
    /**
     * The object is constructed on the zone `z`.
     * See https://github.com/msgpack/msgpack-c/wiki/v1_1_cpp_object
     *
     * @tparam T The type of `v`.
     * @param v The value you want to convert.
     * @param z The zone that is used by the object.
     */
    template <typename T>
    object(const T& v, msgpack::zone& z):v1::object(v, z) {}

public:
    /// Convert the object
    /**
     * If the object can't be converted to T, msgpack::type_error would be thrown.
     * @tparam T The type of v.
     * @param v The value you want to get. `v` is output parameter. `v` is overwritten by converted value from the object.
     * @return The reference of `v`.
     */
    template <typename T>
    T& convert(T& v) const { return v1::object::convert(v); }

    using v1::object::with_zone;
    implicit_type convert() const;
};

/// @cond
} // MSGPACK_API_VERSION_NAMESPACE(v2)
/// @endcond

} // namespace msgpack

#endif // MSGPACK_V2_OBJECT_FWD_HPP
