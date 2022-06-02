//
// MessagePack for C++ static resolution routine
//
// Copyright (C) 2008-2015 FURUHASHI Sadayuki and KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "runtime/msgpack/adaptor/adaptor_base.h"

#include <tuple>

namespace msgpack {

template<typename Tuple, std::size_t N>
struct StdTupleConverter {
  static void convert(const msgpack::object &o, Tuple &v) {
    StdTupleConverter<Tuple, N - 1>::convert(o, v);
    if (o.via.array.size >= N)
      o.via.array.ptr[N - 1].convert<typename std::remove_reference<decltype(std::get<N - 1>(v))>::type>(std::get<N - 1>(v));
  }
};

template<typename Tuple>
struct StdTupleConverter<Tuple, 0> {
  static void convert(const msgpack::object & /*o*/, Tuple & /*v*/) {}
};

template<size_t N>
struct PackValueHelper {
  template<class StreamT, class TupleT>
  static void convert(packer<StreamT> &packer, const TupleT &value) {
    PackValueHelper<N - 1>::convert(packer, value);
    packer_float32_decorator::pack_value(packer, std::get<N - 1>(value));
  }
};

template<>
struct PackValueHelper<0> {
  template<class StreamT, class TupleT>
  static void convert(packer<StreamT> & /*o*/, const TupleT & /*v*/) {}
};

namespace adaptor {

template<typename... Args>
struct convert<std::tuple<Args...>> {
  void operator()(const msgpack::object &o, std::tuple<Args...> &v) const {
    if (o.type != msgpack::type::ARRAY) {
      throw msgpack::type_error();
    }
    StdTupleConverter<decltype(v), sizeof...(Args)>::convert(o, v);
  }
};

template<typename... Args>
struct pack<std::tuple<Args...>> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, const std::tuple<Args...> &v) const {
    o.pack_array(sizeof...(Args));
    PackValueHelper<sizeof...(Args)>::convert(o, v);
  }
};

} // namespace adaptor

} // namespace msgpack
