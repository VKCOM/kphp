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

#include "runtime/msgpack/object_fwd.h"
#include "runtime/msgpack/pack.h"

namespace msgpack {

namespace adaptor {

// Adaptor functors

template <typename T, typename Enabler = void>
struct convert {
    msgpack::object const& operator()(msgpack::object const& o, T& v) const;
};

template <typename T, typename Enabler = void>
struct pack {
    template <typename Stream>
    msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, T const& v) const;
};

} // namespace adaptor

} // namespace msgpack
