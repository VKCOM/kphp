// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "runtime/kphp_core.h"
#include "runtime/msgpack/object.h"
#include "runtime/msgpack/zone.h"

namespace vk::msgpack {

class unpacker : private vk::not_copyable {
public:
  explicit unpacker(const string &input) noexcept
    : input_(input) {}

  msgpack::object unpack();
  bool has_error() const noexcept;
  string get_error_msg() const noexcept;

private:
  const string &input_;
  std::size_t bytes_consumed_{0};
  msgpack::zone zone_;
};

} // namespace vk::msgpack
