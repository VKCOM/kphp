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

template<bool Signed>
struct object_char_sign;

template<>
struct object_char_sign<true> {
  template<typename T>
  static typename std::enable_if<std::is_same<T, char>::value>::type make(msgpack::object &o, T v) {
    if (v < 0) {
      o.type = msgpack::type::NEGATIVE_INTEGER;
      o.via.i64 = v;
    } else {
      o.type = msgpack::type::POSITIVE_INTEGER;
      o.via.u64 = v;
    }
  }
};

template<>
struct object_char_sign<false> {
  static void make(msgpack::object &o, char v) {
    o.type = msgpack::type::POSITIVE_INTEGER;
    o.via.u64 = v;
  }
};

inline void object_char(msgpack::object &o, char v) {
  return object_char_sign<is_signed<char>::value>::make(o, v);
}

} // namespace detail

namespace adaptor {

template<>
struct convert<char> {
  void operator()(const msgpack::object &o, char &v) const {
    v = detail::convert_integer<char>(o);
  }
};

template<>
struct convert<signed char> {
  void operator()(const msgpack::object &o, signed char &v) const {
    v = detail::convert_integer<signed char>(o);
  }
};

template<>
struct convert<signed short> {
  void operator()(const msgpack::object &o, signed short &v) const {
    v = detail::convert_integer<signed short>(o);
  }
};

template<>
struct convert<signed int> {
  void operator()(const msgpack::object &o, signed int &v) const {
    v = detail::convert_integer<signed int>(o);
  }
};

template<>
struct convert<signed long> {
  void operator()(const msgpack::object &o, signed long &v) const {
    v = detail::convert_integer<signed long>(o);
  }
};

template<>
struct convert<signed long long> {
  void operator()(const msgpack::object &o, signed long long &v) const {
    v = detail::convert_integer<signed long long>(o);
  }
};

template<>
struct convert<unsigned char> {
  void operator()(const msgpack::object &o, unsigned char &v) const {
    v = detail::convert_integer<unsigned char>(o);
  }
};

template<>
struct convert<unsigned short> {
  void operator()(const msgpack::object &o, unsigned short &v) const {
    v = detail::convert_integer<unsigned short>(o);
  }
};

template<>
struct convert<unsigned int> {
  void operator()(const msgpack::object &o, unsigned int &v) const {
    v = detail::convert_integer<unsigned int>(o);
  }
};

template<>
struct convert<unsigned long> {
  void operator()(const msgpack::object &o, unsigned long &v) const {
    v = detail::convert_integer<unsigned long>(o);
  }
};

template<>
struct convert<unsigned long long> {
  void operator()(const msgpack::object &o, unsigned long long &v) const {
    v = detail::convert_integer<unsigned long long>(o);
  }
};

template<>
struct pack<char> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, char v) const {
    o.pack_char(v);
  }
};

template<>
struct pack<signed char> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, signed char v) const {
    o.pack_signed_char(v);
  }
};

template<>
struct pack<signed short> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, signed short v) const {
    o.pack_short(v);
  }
};

template<>
struct pack<signed int> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, signed int v) const {
    o.pack_int(v);
  }
};

template<>
struct pack<signed long> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, signed long v) const {
    o.pack_long(v);
  }
};

template<>
struct pack<signed long long> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, signed long long v) const {
    o.pack_long_long(v);
  }
};

template<>
struct pack<unsigned char> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, unsigned char v) const {
    o.pack_unsigned_char(v);
  }
};

template<>
struct pack<unsigned short> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, unsigned short v) const {
    o.pack_unsigned_short(v);
  }
};

template<>
struct pack<unsigned int> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, unsigned int v) const {
    o.pack_unsigned_int(v);
  }
};

template<>
struct pack<unsigned long> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, unsigned long v) const {
    o.pack_unsigned_long(v);
  }
};

template<>
struct pack<unsigned long long> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, unsigned long long v) const {
    o.pack_unsigned_long_long(v);
  }
};

} // namespace adaptor

} // namespace msgpack
