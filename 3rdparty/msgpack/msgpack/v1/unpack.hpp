//
// MessagePack for C++ deserializing routine
//
// Copyright (C) 2008-2016 FURUHASHI Sadayuki and KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef MSGPACK_V1_UNPACK_HPP
#define MSGPACK_V1_UNPACK_HPP

#include "msgpack/versioning.hpp"
#include "msgpack/unpack_decl.hpp"
#include "msgpack/object.hpp"
#include "msgpack/zone.hpp"
#include "msgpack/unpack_exception.hpp"
#include "msgpack/unpack_define.h"
#include "msgpack/cpp_config.hpp"
#include "msgpack/sysdep.h"

#include <memory>

#if !defined(MSGPACK_USE_CPP03)
#include <atomic>
#endif


#if defined(_MSC_VER)
// avoiding confliction std::max, std::min, and macro in windows.h
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif // defined(_MSC_VER)

namespace msgpack {

/// @cond
MSGPACK_API_VERSION_NAMESPACE(v1) {
/// @endcond

namespace detail {

class unpack_user {
public:
    unpack_user(unpack_reference_func f = MSGPACK_NULLPTR,
                void* user_data = MSGPACK_NULLPTR,
                unpack_limit const& limit = unpack_limit())
        :m_func(f), m_user_data(user_data), m_limit(limit) {}
    msgpack::zone const& zone() const { return *m_zone; }
    msgpack::zone& zone() { return *m_zone; }
    void set_zone(msgpack::zone& zone) { m_zone = &zone; }
    bool referenced() const { return m_referenced; }
    void set_referenced(bool referenced) { m_referenced = referenced; }
    unpack_reference_func reference_func() const { return m_func; }
    void* user_data() const { return m_user_data; }
    unpack_limit const& limit() const { return m_limit; }
    unpack_limit& limit() { return m_limit; }

private:
    msgpack::zone* m_zone;
    bool m_referenced;
    unpack_reference_func m_func;
    void* m_user_data;
    unpack_limit m_limit;
};

inline void unpack_uint8(uint8_t d, msgpack::object& o)
{ o.type = msgpack::type::POSITIVE_INTEGER; o.via.u64 = d; }

inline void unpack_uint16(uint16_t d, msgpack::object& o)
{ o.type = msgpack::type::POSITIVE_INTEGER; o.via.u64 = d; }

inline void unpack_uint32(uint32_t d, msgpack::object& o)
{ o.type = msgpack::type::POSITIVE_INTEGER; o.via.u64 = d; }

inline void unpack_uint64(uint64_t d, msgpack::object& o)
{ o.type = msgpack::type::POSITIVE_INTEGER; o.via.u64 = d; }

inline void unpack_int8(int8_t d, msgpack::object& o)
{ if(d >= 0) { o.type = msgpack::type::POSITIVE_INTEGER; o.via.u64 = d; }
        else { o.type = msgpack::type::NEGATIVE_INTEGER; o.via.i64 = d; } }

inline void unpack_int16(int16_t d, msgpack::object& o)
{ if(d >= 0) { o.type = msgpack::type::POSITIVE_INTEGER; o.via.u64 = d; }
        else { o.type = msgpack::type::NEGATIVE_INTEGER; o.via.i64 = d; } }

inline void unpack_int32(int32_t d, msgpack::object& o)
{ if(d >= 0) { o.type = msgpack::type::POSITIVE_INTEGER; o.via.u64 = d; }
        else { o.type = msgpack::type::NEGATIVE_INTEGER; o.via.i64 = d; } }

inline void unpack_int64(int64_t d, msgpack::object& o)
{ if(d >= 0) { o.type = msgpack::type::POSITIVE_INTEGER; o.via.u64 = d; }
        else { o.type = msgpack::type::NEGATIVE_INTEGER; o.via.i64 = d; } }

inline void unpack_float(float d, msgpack::object& o)
{ o.type = msgpack::type::FLOAT32; o.via.f64 = d; }

inline void unpack_double(double d, msgpack::object& o)
{ o.type = msgpack::type::FLOAT64; o.via.f64 = d; }

inline void unpack_nil(msgpack::object& o)
{ o.type = msgpack::type::NIL; }

inline void unpack_true(msgpack::object& o)
{ o.type = msgpack::type::BOOLEAN; o.via.boolean = true; }

inline void unpack_false(msgpack::object& o)
{ o.type = msgpack::type::BOOLEAN; o.via.boolean = false; }

template <typename T>
struct value {
    typedef T type;
};
template <>
struct value<fix_tag> {
    typedef uint32_t type;
};

template <typename T>
inline typename msgpack::enable_if<sizeof(T) == sizeof(fix_tag)>::type load(uint32_t& dst, const char* n) {
    dst = static_cast<uint32_t>(*reinterpret_cast<const uint8_t*>(n)) & 0x0f;
}

template <typename T>
inline typename msgpack::enable_if<sizeof(T) == 1>::type load(T& dst, const char* n) {
    dst = static_cast<T>(*reinterpret_cast<const uint8_t*>(n));
}

template <typename T>
inline typename msgpack::enable_if<sizeof(T) == 2>::type load(T& dst, const char* n) {
    _msgpack_load16(T, n, &dst);
}

template <typename T>
inline typename msgpack::enable_if<sizeof(T) == 4>::type load(T& dst, const char* n) {
    _msgpack_load32(T, n, &dst);
}

template <typename T>
inline typename msgpack::enable_if<sizeof(T) == 8>::type load(T& dst, const char* n) {
    _msgpack_load64(T, n, &dst);
}

} // detail


/// @cond
}  // MSGPACK_API_VERSION_NAMESPACE(v1)
/// @endcond

}  // namespace msgpack


#endif // MSGPACK_V1_UNPACK_HPP
