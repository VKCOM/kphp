// Compiler for PHP (aka KPHP)
// msgpack (c) https://github.com/msgpack/msgpack-c/tree/cpp_master (copied as third-party and slightly modified)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/msgpack/unpacker.h"

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/msgpack/object_visitor.h"
#include "runtime-common/stdlib/msgpack/parser.h"

namespace vk::msgpack {

msgpack::object unpacker::unpack() {
  object_visitor visitor{zone_};
  if (zone_.has_error()) {
    return {};
  }
  parse_return ret = parser<object_visitor>::parse(input_.c_str(), input_.size(), bytes_consumed_, visitor);
  if (visitor.has_error()) {
    visitor_error_ = true;
    return {};
  }
  // TODO visitor may store error, should check here

  switch (ret) {
  case parse_return::SUCCESS:
  case parse_return::EXTRA_BYTES:
    return std::move(visitor).flush();
  default:
    // For no it's assumed as unreachable because of throwing exception mechanism
    // Good place to load error and store it somewhere
    // Looks like store right in unpacker
    assert(false && "unreachable");
    return {};
  }
}

bool unpacker::has_error() const noexcept {
  return RuntimeContext::get().msgpack_error.has_value() || visitor_error_ || zone_.has_error() || bytes_consumed_ != input_.size();
}

string unpacker::get_error_msg() const noexcept {
  if (RuntimeContext::get().msgpack_error.has_value()) {
    return string(RuntimeContext::get().msgpack_error->data());
  }
  if (visitor_error_) {
    return string("visitor error");
  }
  if (zone_.has_error()) {
    return string("cannot allocate");
  }
  string error;
  if (bytes_consumed_ != input_.size()) {
    error.append("Consumed only first ")
        .append(static_cast<int64_t>(bytes_consumed_))
        .append(" characters of ")
        .append(static_cast<int64_t>(input_.size()))
        .append(" during deserialization");
  }
  return error;
}

} // namespace vk::msgpack
