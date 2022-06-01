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

template<typename T>
struct convert {
  void operator()(const msgpack::object &o, T &v) const {
    v.msgpack_unpack(o);
  }
};

template<typename T>
struct pack {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, const T &v) const {
    v.msgpack_pack(o);
  }
};

} // namespace adaptor

class type_error : public std::bad_cast {};

} // namespace msgpack
