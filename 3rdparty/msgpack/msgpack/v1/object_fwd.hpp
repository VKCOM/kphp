//
// MessagePack for C++ static resolution routine
//
// Copyright (C) 2008-2014 FURUHASHI Sadayuki and KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MSGPACK_V1_OBJECT_FWD_HPP
#define MSGPACK_V1_OBJECT_FWD_HPP

#include "msgpack/object_fwd_decl.hpp"

namespace msgpack {

/// @cond
MSGPACK_API_VERSION_NAMESPACE(v1) {
/// @endcond

struct object_array {
    uint32_t size;
    msgpack::object* ptr;
};

struct object_map {
    uint32_t size;
    msgpack::object_kv* ptr;
};

struct object_str {
    uint32_t size;
    const char* ptr;
};

struct object_bin {
    uint32_t size;
    const char* ptr;
};

struct object_ext {
    int8_t type() const { return ptr[0]; }
    const char* data() const { return &ptr[1]; }
    uint32_t size;
    const char* ptr;
};

/// Object class that corresponding to MessagePack format object
/**
 * See https://github.com/msgpack/msgpack-c/wiki/v1_1_cpp_object
 */
struct object {
    union union_type {
        bool boolean;
        uint64_t u64;
        int64_t  i64;
#if defined(MSGPACK_USE_LEGACY_NAME_AS_FLOAT)
        MSGPACK_DEPRECATED("please use f64 instead")
        double   dec; // obsolete
#endif // MSGPACK_USE_LEGACY_NAME_AS_FLOAT
        double   f64;
        msgpack::object_array array;
        msgpack::object_map map;
        msgpack::object_str str;
        msgpack::object_bin bin;
        msgpack::object_ext ext;
    };

    msgpack::type::object_type type{type::NIL};
    union_type via;

    /// Get value as T
    /**
     * If the object can't be converted to T, msgpack::type_error would be thrown.
     * @tparam T The type you want to get.
     * @return The converted object.
     */
    template <typename T>
    T as() const;

    /// Convert the object
    /**
     * If the object can't be converted to T, msgpack::type_error would be thrown.
     * @tparam T The type of v.
     * @param v The value you want to get. `v` is output parameter. `v` is overwritten by converted value from the object.
     * @return The reference of `v`.
     */
    template <typename T>
    typename msgpack::enable_if<
        !msgpack::is_array<T>::value && !msgpack::is_pointer<T>::value,
        T&
    >::type
    convert(T& v) const;
};

class type_error : public std::bad_cast { };

/// @cond
} // MSGPACK_API_VERSION_NAMESPACE(v1)
/// @endcond

} // namespace msgpack

#endif // MSGPACK_V1_OBJECT_FWD_HPP
