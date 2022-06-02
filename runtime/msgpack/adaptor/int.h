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
#include "runtime/msgpack/object.h"

#include <limits>

namespace msgpack {

namespace detail {

template<typename T, bool Signed>
struct convert_integer_sign;

template<typename T>
struct convert_integer_sign<T, true> {
  static T convert(const msgpack::object &o) {
    if (o.type == msgpack::type::POSITIVE_INTEGER) {
      if (o.via.u64 > static_cast<uint64_t>(std::numeric_limits<T>::max())) {
        throw msgpack::type_error();
      }
      return static_cast<T>(o.via.u64);
    } else if (o.type == msgpack::type::NEGATIVE_INTEGER) {
      if (o.via.i64 < static_cast<int64_t>(std::numeric_limits<T>::min())) {
        throw msgpack::type_error();
      }
      return static_cast<T>(o.via.i64);
    }
    throw msgpack::type_error();
  }
};

template<typename T>
struct convert_integer_sign<T, false> {
  static T convert(const msgpack::object &o) {
    if (o.type == msgpack::type::POSITIVE_INTEGER) {
      if (o.via.u64 > static_cast<uint64_t>(std::numeric_limits<T>::max())) {
        throw msgpack::type_error();
      }
      return static_cast<T>(o.via.u64);
    }
    throw msgpack::type_error();
  }
};

template<typename T>
struct is_signed {
  static const bool value = std::numeric_limits<T>::is_signed;
};

template<typename T>
inline T convert_integer(const msgpack::object &o) {
  return detail::convert_integer_sign<T, is_signed<T>::value>::convert(o);
}

} // namespace detail

namespace adaptor {

template<>
struct convert<int32_t> {
  void operator()(const msgpack::object &o, int32_t &v) const {
    v = detail::convert_integer<int32_t>(o);
  }
};

template<>
struct convert<int64_t> {
  void operator()(const msgpack::object &o, int64_t &v) const {
    v = detail::convert_integer<int64_t>(o);
  }
};

template<>
struct convert<uint8_t> {
  void operator()(const msgpack::object &o, uint8_t &v) const {
    v = detail::convert_integer<uint8_t>(o);
  }
};

template<>
struct convert<uint32_t> {
  void operator()(const msgpack::object &o, uint32_t &v) const {
    v = detail::convert_integer<uint32_t>(o);
  }
};

template<>
struct convert<uint64_t> {
  void operator()(const msgpack::object &o, uint64_t &v) const {
    v = detail::convert_integer<uint64_t>(o);
  }
};

template<>
struct pack<int32_t> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, int32_t v) const {
    o.pack_int32(v);
  }
};

template<>
struct pack<int64_t> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, int64_t v) const {
    o.pack_int64(v);
  }
};

template<>
struct pack<uint8_t> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, uint8_t v) const {
    o.pack_uint8(v);
  }
};

template<>
struct pack<uint32_t> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, uint32_t v) const {
    o.pack_uint32(v);
  }
};

template<>
struct pack<uint64_t> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, uint64_t v) const {
    o.pack_uint64(v);
  }
};

} // namespace adaptor

} // namespace msgpack
