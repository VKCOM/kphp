// Compiler for PHP (aka KPHP)
// msgpack (c) https://github.com/msgpack/msgpack-c/tree/cpp_master (copied as third-party and slightly modified)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/msgpack/unpacker.h"

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/msgpack/object_visitor.h"
#include "runtime-common/stdlib/msgpack/parser.h"
#include "runtime-common/stdlib/serialization/serialization-context.h"

namespace vk::msgpack {

msgpack::object unpacker::unpack() {
  object_visitor visitor{zone_};
  if (zone_.has_error()) {
    return {};
  }
  parse_return ret = parser<object_visitor>::parse(input_.c_str(), input_.size(), bytes_consumed_, visitor);
  visitor_error_ = visitor.get_error();

  switch (ret) {
  case parse_return::SUCCESS:
  case parse_return::EXTRA_BYTES:
    return std::move(visitor).flush();
  default:
    // Error happened
    return {};
  }
}

bool unpacker::has_error() const noexcept {
  return SerializationLibContext::get().msgpack_error.has_value() || visitor_error_.has_value() || zone_.has_error() || bytes_consumed_ != input_.size();
}

string unpacker::get_error_msg() const noexcept {
  const auto& serialization_ctx = SerializationLibContext::get();
  if (serialization_ctx.msgpack_error.has_value()) {
    return {serialization_ctx.msgpack_error->data(), static_cast<uint32_t>(serialization_ctx.msgpack_error->size())};
  }
  if (visitor_error_.has_value()) {
    return {visitor_error_->data(), static_cast<uint32_t>(visitor_error_->size())};
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
