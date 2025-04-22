// Compiler for PHP (aka KPHP)
// msgpack (c) https://github.com/msgpack/msgpack-c/tree/cpp_master (copied as third-party and slightly modified)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "common/wrappers/string_view.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/msgpack/object.h"
#include "runtime-common/stdlib/msgpack/zone.h"

namespace vk::msgpack {

class unpacker : private vk::not_copyable {
public:
  explicit unpacker(const string& input) noexcept
      : input_(input) {}

  msgpack::object unpack();
  bool has_error() const noexcept;
  string get_error_msg() const noexcept;

private:
  const string& input_;
  std::size_t bytes_consumed_{0};
  msgpack::zone zone_;

  // It's awful because unpacker and visitor are strongly logically connected,
  // but there is no aggregation or composition. Should be fixed some day.
  std::optional<vk::string_view> visitor_error_{std::nullopt};
};

} // namespace vk::msgpack
