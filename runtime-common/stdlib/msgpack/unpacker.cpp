// Compiler for PHP (aka KPHP)
// msgpack (c) https://github.com/msgpack/msgpack-c/tree/cpp_master (copied as third-party and slightly modified)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/msgpack/unpacker.h"

#include "runtime-common/stdlib/msgpack/object_visitor.h"
#include "runtime-common/stdlib/msgpack/parser.h"

namespace vk::msgpack {

msgpack::object unpacker::unpack() {
  object_visitor visitor{zone_};
  parse_return ret = parser<object_visitor>::parse(input_.c_str(), input_.size(), bytes_consumed_, visitor);

  switch (ret) {
  case parse_return::SUCCESS:
  case parse_return::EXTRA_BYTES:
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
    error.append("Consumed only first ")
        .append(static_cast<int64_t>(bytes_consumed_))
        .append(" characters of ")
        .append(static_cast<int64_t>(input_.size()))
        .append(" during deserialization");
  }
  return error;
}

} // namespace vk::msgpack
