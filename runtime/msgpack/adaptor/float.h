//
// MessagePack for C++ static resolution routine
//
// Copyright (C) 2008-2009 FURUHASHI Sadayuki
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "runtime/msgpack/object.h"

namespace msgpack {

// FIXME check overflow, underflow

namespace adaptor {

template<>
struct convert<float> {
  void operator()(const msgpack::object &o, float &v) const {
    if (o.type == msgpack::type::FLOAT32 || o.type == msgpack::type::FLOAT64) {
      v = static_cast<float>(o.via.f64);
    } else if (o.type == msgpack::type::POSITIVE_INTEGER) {
      v = static_cast<float>(o.via.u64);
    } else if (o.type == msgpack::type::NEGATIVE_INTEGER) {
      v = static_cast<float>(o.via.i64);
    } else {
      throw msgpack::type_error();
    }
  }
};

template<>
struct pack<float> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, const float &v) const {
    o.pack_float(v);
  }
};

template<>
struct convert<double> {
  void operator()(const msgpack::object &o, double &v) const {
    if (o.type == msgpack::type::FLOAT32 || o.type == msgpack::type::FLOAT64) {
      v = o.via.f64;
    } else if (o.type == msgpack::type::POSITIVE_INTEGER) {
      v = static_cast<double>(o.via.u64);
    } else if (o.type == msgpack::type::NEGATIVE_INTEGER) {
      v = static_cast<double>(o.via.i64);
    } else {
      throw msgpack::type_error();
    }
  }
};

template<>
struct pack<double> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, const double &v) const {
    o.pack_double(v);
  }
};

} // namespace adaptor

} // namespace msgpack
