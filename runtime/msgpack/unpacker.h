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

#include "common/mixin/not_copyable.h"
#include "runtime/kphp_core.h"
#include "runtime/msgpack/object.h"
#include "runtime/msgpack/zone.h"

namespace msgpack {

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

} // namespace msgpack
