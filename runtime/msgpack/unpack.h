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

inline msgpack::object_handle unpack(const char* data, std::size_t len, std::size_t& off) {
  auto z = std::make_unique<msgpack::zone>();
  create_object_visitor visitor{*z};
  parse_return ret = detail::parse_imp(data, len, off, visitor);

  switch(ret) {
    case msgpack::PARSE_SUCCESS:
    case msgpack::PARSE_EXTRA_BYTES:
      return {std::move(visitor).flush(), std::move(z)};
    default:
      return {};
  }
}

}  // namespace msgpack
