//
// MessagePack for C++ deserializing routine
//
// Copyright (C) 2008-2016 FURUHASHI Sadayuki and KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef MSGPACK_V1_UNPACK_DECL_HPP
#define MSGPACK_V1_UNPACK_DECL_HPP

#include "msgpack/versioning.hpp"
#include "msgpack/unpack_define.h"
#include "msgpack/v1/object.hpp"
#include "msgpack/zone.hpp"
#include "msgpack/cpp_config.hpp"
#include "msgpack/sysdep.h"
#include "msgpack/parse_return.hpp"

#include <memory>
#include <stdexcept>

#if !defined(MSGPACK_USE_CPP03)
#include <atomic>
#endif


#if defined(_MSC_VER)
// avoiding confliction std::max, std::min, and macro in windows.h
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif // defined(_MSC_VER)

#ifdef _msgpack_atomic_counter_header
#include _msgpack_atomic_counter_header
#endif

const size_t COUNTER_SIZE = sizeof(_msgpack_atomic_counter_t);

#ifndef MSGPACK_UNPACKER_INIT_BUFFER_SIZE
#define MSGPACK_UNPACKER_INIT_BUFFER_SIZE (64*1024)
#endif

#ifndef MSGPACK_UNPACKER_RESERVE_SIZE
#define MSGPACK_UNPACKER_RESERVE_SIZE (32*1024)
#endif


// backward compatibility
#ifndef MSGPACK_UNPACKER_DEFAULT_INITIAL_BUFFER_SIZE
#define MSGPACK_UNPACKER_DEFAULT_INITIAL_BUFFER_SIZE MSGPACK_UNPACKER_INIT_BUFFER_SIZE
#endif


namespace msgpack {

/// @cond
MSGPACK_API_VERSION_NAMESPACE(v1) {
/// @endcond

/// The type of reference or copy judging function.
/**
 * @param type msgpack data type.
 * @param size msgpack data size.
 * @param user_data The user_data that is set by msgpack::unpack functions.
 *
 * @return If the data should be referenced, then return true, otherwise (should be copied) false.
 *
 * This function is called when unpacking STR, BIN, or EXT.
 *
 */
typedef bool (*unpack_reference_func)(msgpack::type::object_type type, std::size_t size, void* user_data);

struct unpack_error;
struct parse_error;
struct insufficient_bytes;
struct size_overflow;
struct array_size_overflow;
struct map_size_overflow;
struct str_size_overflow;
struct bin_size_overflow;
struct ext_size_overflow;
struct depth_size_overflow;

class unpack_limit {
public:
    unpack_limit(
        std::size_t array = 0xffffffff,
        std::size_t map = 0xffffffff,
        std::size_t str = 0xffffffff,
        std::size_t bin = 0xffffffff,
        std::size_t ext = 0xffffffff,
        std::size_t depth = 0xffffffff)
        :array_(array),
         map_(map),
         str_(str),
         bin_(bin),
         ext_(ext),
         depth_(depth) {}
    std::size_t array() const { return array_; }
    std::size_t map() const { return map_; }
    std::size_t str() const { return str_; }
    std::size_t bin() const { return bin_; }
    std::size_t ext() const { return ext_; }
    std::size_t depth() const { return depth_; }

private:
    std::size_t array_;
    std::size_t map_;
    std::size_t str_;
    std::size_t bin_;
    std::size_t ext_;
    std::size_t depth_;
};

namespace detail {

struct fix_tag {
    char f1[65]; // FIXME unique size is required. or use is_same meta function.
};

template <typename T>
struct value;

template <typename T>
typename msgpack::enable_if<sizeof(T) == sizeof(fix_tag)>::type load(uint32_t& dst, const char* n);

template <typename T>
typename msgpack::enable_if<sizeof(T) == 1>::type load(T& dst, const char* n);

template <typename T>
typename msgpack::enable_if<sizeof(T) == 2>::type load(T& dst, const char* n);

template <typename T>
typename msgpack::enable_if<sizeof(T) == 4>::type load(T& dst, const char* n);

template <typename T>
typename msgpack::enable_if<sizeof(T) == 8>::type load(T& dst, const char* n);

} // detail

/// @cond
}  // MSGPACK_API_VERSION_NAMESPACE(v1)
/// @endcond

}  // namespace msgpack

#endif // MSGPACK_V1_UNPACK_DECL_HPP
