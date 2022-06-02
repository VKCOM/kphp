// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"
#include "runtime/msgpack/adaptor/adaptor_base.h"
#include "runtime/msgpack/object.h"

namespace msgpack::adaptor {

template<>
struct convert<string> {
  const msgpack::object &operator()(const msgpack::object &obj, string &res_s) const {
    if (obj.type != msgpack::type::STR) {
      throw msgpack::type_error();
    }
    res_s = string(obj.via.str.ptr, obj.via.str.size);

    return obj;
  }
};

template<>
struct pack<string> {
  template<typename Stream>
  packer<Stream> &operator()(msgpack::packer<Stream> &packer, const string &s) const noexcept {
    packer.pack_str(s.size());
    packer.pack_str_body(s.c_str(), s.size());
    return packer;
  }
};

} // namespace msgpack::adaptor
