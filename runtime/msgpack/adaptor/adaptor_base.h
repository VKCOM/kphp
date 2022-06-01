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

#include <stdexcept>

namespace msgpack {

struct object;

template<typename Stream>
class packer;

namespace adaptor {

// Adaptor functors

template<typename T, typename Enabler = void>
struct convert {
  msgpack::object const &operator()(msgpack::object const &o, T &v) const {
    v.msgpack_unpack(o);
    return o;
  }
};

template<typename T, typename Enabler = void>
struct pack {
  template<typename Stream>
  msgpack::packer<Stream> &operator()(msgpack::packer<Stream> &o, const T &v) const {
    v.msgpack_pack(o);
    return o;
  }
};

} // namespace adaptor

class type_error : public std::bad_cast {};

} // namespace msgpack
