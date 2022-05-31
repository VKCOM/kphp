//
// MessagePack for C++ deserializing routine
//
// Copyright (C) 2016 KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//

#include "runtime/msgpack/unpacker.h"

#include "runtime/msgpack/parser.h"
#include "runtime/msgpack/object_visitor.h"

namespace msgpack {

msgpack::object unpacker::unpack() {
  object_visitor visitor{zone_};
  parse_return ret = parser<object_visitor>::parse(input_.c_str(), input_.size(), bytes_consumed_, visitor);

  switch(ret) {
    case msgpack::PARSE_SUCCESS:
    case msgpack::PARSE_EXTRA_BYTES:
      return std::move(visitor).flush();
    default:
      return {};
  }
}

bool unpacker::has_error() const noexcept {
  return bytes_consumed_ != input_.size();
}

string unpacker::get_error_msg() const noexcept {
  string error;
  if (has_error()) {
    error.append("Consumed only first ").append(static_cast<int64_t>(bytes_consumed_))
      .append(" characters of ").append(static_cast<int64_t>(input_.size()))
      .append(" during deserialization");
  }
  return error;
}

}  // namespace msgpack
