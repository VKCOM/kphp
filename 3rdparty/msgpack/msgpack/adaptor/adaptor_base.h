//
// MessagePack for C++ static resolution routine
//
// Copyright (C) 2015-2016 KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "msgpack/adaptor/adaptor_base_decl.h"

namespace msgpack {

namespace adaptor {

// Adaptor functors

template <typename T, typename Enabler>
struct convert {
    msgpack::object const& operator()(msgpack::object const& o, T& v) const;
};

template <typename T, typename Enabler>
struct pack {
    template <typename Stream>
    msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, T const& v) const;
};

} // namespace adaptor

// operators

template <typename T>
inline msgpack::object const& operator>> (msgpack::object const& o, T& v) {
    return msgpack::adaptor::convert<T>()(o, v);
}

template <typename Stream, typename T>
inline msgpack::packer<Stream>& operator<< (msgpack::packer<Stream>& o, T const& v) {
    return msgpack::adaptor::pack<T>()(o, v);
}

} // namespace msgpack
