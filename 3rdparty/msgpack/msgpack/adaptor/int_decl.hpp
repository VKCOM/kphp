//
// MessagePack for C++ static resolution routine
//
// Copyright (C) 2008-2016 FURUHASHI Sadayuki and KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "msgpack/adaptor/adaptor_base.hpp"
#include <limits>

namespace msgpack {

namespace type {
namespace detail {

template <typename T, bool Signed>
struct convert_integer_sign;

template <typename T>
struct is_signed;

template <typename T>
T convert_integer(msgpack::object const& o);

template <bool Signed>
struct object_char_sign;

void object_char(msgpack::object& o, char v);

}  // namespace detail
}  // namespace type

}  // namespace msgpack
