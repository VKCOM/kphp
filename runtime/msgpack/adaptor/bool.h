// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/msgpack/adaptor/adaptor_base.h"
#include "runtime/msgpack/object.h"

namespace vk::msgpack::adaptor {

template<>
struct convert<bool> {
  void operator()(const msgpack::object &o, bool &v) const {
    if (o.type != msgpack::type::BOOLEAN) {
      throw msgpack::type_error();
    }
    v = o.via.boolean;
  }
};

template<>
struct pack<bool> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, const bool &v) const {
    if (v) {
      o.pack_true();
    } else {
      o.pack_false();
    }
  }
};

} // namespace vk::msgpack::adaptor
