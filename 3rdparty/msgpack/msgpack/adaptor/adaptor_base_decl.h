//
// MessagePack for C++ static resolution routine
//
// Copyright (C) 2016 KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "msgpack/object_fwd.h"
#include "msgpack/pack.h"

namespace msgpack {

template <typename Stream>
class packer;

namespace adaptor {

// Adaptor functors

template <typename T, typename Enabler = void>
struct convert;

template <typename T, typename Enabler = void>
struct pack;

} // namespace adaptor

// operators

template <typename T>
msgpack::object const& operator>> (msgpack::object const& o, T& v);

template <typename Stream, typename T>
msgpack::packer<Stream>& operator<< (msgpack::packer<Stream>& o, T const& v);

} // namespace msgpack
