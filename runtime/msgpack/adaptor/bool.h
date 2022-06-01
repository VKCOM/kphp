//
// MessagePack for C++ static resolution routine
//
// Copyright (C) 2008-2016 FURUHASHI Sadayuki
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "runtime/msgpack/adaptor/adaptor_base.h"
#include "runtime/msgpack/object_fwd.h"

namespace msgpack {

namespace adaptor {

template<>
struct convert<bool> {
  msgpack::object const &operator()(msgpack::object const &o, bool &v) const {
    if (o.type != msgpack::type::BOOLEAN) {
      throw msgpack::type_error();
    }
    v = o.via.boolean;
    return o;
  }
};

template<>
struct pack<bool> {
  template<typename Stream>
  msgpack::packer<Stream> &operator()(msgpack::packer<Stream> &o, const bool &v) const {
    if (v) {
      o.pack_true();
    } else {
      o.pack_false();
    }
    return o;
  }
};

} // namespace adaptor

} // namespace msgpack
