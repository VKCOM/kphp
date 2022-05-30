//
// MessagePack for C++ deserializing routine
//
// Copyright (C) 2008-2016 FURUHASHI Sadayuki and KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef MSGPACK_V1_UNPACK_EXCEPTION_HPP
#define MSGPACK_V1_UNPACK_EXCEPTION_HPP

#include "msgpack/versioning.hpp"

#include <string>
#include <stdexcept>


namespace msgpack {

/// @cond
MSGPACK_API_VERSION_NAMESPACE(v3) {
/// @endcond

struct unpack_error : public std::runtime_error {
    explicit unpack_error(const std::string& msg)
        :std::runtime_error(msg) {}
};

struct parse_error : public unpack_error {
    explicit parse_error(const std::string& msg)
        :unpack_error(msg) {}
};

struct insufficient_bytes : public unpack_error {
    explicit insufficient_bytes(const std::string& msg)
        :unpack_error(msg) {}
};

struct size_overflow : public unpack_error {
    explicit size_overflow(const std::string& msg)
        :unpack_error(msg) {}
};

struct array_size_overflow : public size_overflow {
    array_size_overflow(const std::string& msg)
        :size_overflow(msg) {}
};

struct map_size_overflow : public size_overflow {
    map_size_overflow(const std::string& msg)
        :size_overflow(msg) {}
};

struct str_size_overflow : public size_overflow {
    str_size_overflow(const std::string& msg)
        :size_overflow(msg) {}
};

struct bin_size_overflow : public size_overflow {
    bin_size_overflow(const std::string& msg)
        :size_overflow(msg) {}
};

struct ext_size_overflow : public size_overflow {
    ext_size_overflow(const std::string& msg)
        :size_overflow(msg) {}
};

struct depth_size_overflow : public size_overflow {
    depth_size_overflow(const std::string& msg)
        :size_overflow(msg) {}
};

/// @cond
}  // MSGPACK_API_VERSION_NAMESPACE(v1)
/// @endcond

}  // namespace msgpack


#endif // MSGPACK_V1_UNPACK_EXCEPTION_HPP
