//
// MessagePack for C++ deserializing routine
//
// Copyright (C) 2016 KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <memory>
#include "runtime/msgpack/parse.h"
#include "runtime/msgpack/create_object_visitor.h"

namespace msgpack {

namespace detail {

inline parse_return
unpack_imp(const char* data, std::size_t len, std::size_t& off,
           msgpack::zone& result_zone, msgpack::object& result)
{
    create_object_visitor v;
    v.set_zone(result_zone);
    parse_return ret = parse_imp(data, len, off, v);
    result = v.data();
    return ret;
}

} // namespace detail

inline msgpack::object_handle unpack(const char* data, std::size_t len, std::size_t& off) {
  msgpack::object obj;
  auto z = std::make_unique<msgpack::zone>();
  parse_return ret = detail::unpack_imp(data, len, off, *z, obj);

  switch(ret) {
    case msgpack::PARSE_SUCCESS:
      return msgpack::object_handle(obj, std::move(z));
    case msgpack::PARSE_EXTRA_BYTES:
      return msgpack::object_handle(obj, std::move(z));
    default:
      break;
  }
  return msgpack::object_handle();
}

}  // namespace msgpack
