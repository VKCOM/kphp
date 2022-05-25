//
// MessagePack for C++ C++03/C++11 Adaptation
//
// Copyright (C) 2013-2016 KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef MSGPACK_CPP_CONFIG_HPP
#define MSGPACK_CPP_CONFIG_HPP

#define MSGPACK_NULLPTR nullptr
#define MSGPACK_DEPRECATED(msg) [[deprecated(msg)]]

#include <memory>
#include <tuple>

namespace msgpack {
using std::unique_ptr;
using std::move;
using std::swap;
using std::enable_if;
using std::is_same;
using std::underlying_type;
using std::is_array;
using std::remove_const;
using std::remove_volatile;
using std::remove_cv;
using std::is_pointer;
} // namespace msgpack

#endif // MSGPACK_CPP_CONFIG_HPP
