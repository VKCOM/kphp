// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/msgpack/adaptor/adaptor_base.h"
#include "runtime/msgpack/object.h"
#include "runtime/kphp_core.h"

namespace vk::msgpack::adaptor {

template<>
struct convert<mixed> {
  const msgpack::object &operator()(const msgpack::object &obj, mixed &v) const {
    switch (obj.type) {
      case msgpack::type::STR:
        v = obj.as<string>();
        break;
      case msgpack::type::ARRAY:
        v = obj.as<array<mixed>>();
        break;
      case msgpack::type::NEGATIVE_INTEGER:
      case msgpack::type::POSITIVE_INTEGER:
        v = obj.as<int64_t>();
        break;
      case msgpack::type::FLOAT32:
      case msgpack::type::FLOAT64:
        v = obj.as<double>();
        break;
      case msgpack::type::BOOLEAN:
        v = obj.as<bool>();
        break;
      case msgpack::type::MAP:
        v = obj.as<array<mixed>>();
        break;
      case msgpack::type::NIL:
        v = mixed{};
        break;
      default:
        throw msgpack::type_error();
    }

    return obj;
  }
};

template<>
struct pack<mixed> {
  template<typename Stream>
  packer<Stream> &operator()(msgpack::packer<Stream> &packer, const mixed &v) const noexcept {
    switch (v.get_type()) {
      case mixed::type::NUL:
        packer.pack_nil();
        break;
      case mixed::type::BOOLEAN:
        packer.pack(v.as_bool());
        break;
      case mixed::type::INTEGER:
        packer.pack(v.as_int());
        break;
      case mixed::type::FLOAT:
        packer_float32_decorator::pack_value(packer, v.as_double());
        break;
      case mixed::type::STRING:
        packer.pack(v.as_string());
        break;
      case mixed::type::ARRAY:
        packer.pack(v.as_array());
        break;
      default:
        __builtin_unreachable();
    }

    return packer;
  }
};

} // namespace vk::msgpack::adaptor
