// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"
#include "runtime/msgpack/adaptor/adaptor_base.h"
#include "runtime/msgpack/object.h"

namespace vk::msgpack::adaptor {

template<class T>
struct convert<Optional<T>> {
  const msgpack::object &operator()(const msgpack::object &obj, Optional<T> &v) const {
    switch (obj.type) {
      case msgpack::type::BOOLEAN: {
        bool value = obj.as<bool>();
        if (!std::is_same<T, bool>{} && value) {
          char err_msg[256];
          snprintf(err_msg, 256, "Expected false for type `%s|false` but true was given", typeid(T).name());
          throw msgpack::unpack_error(err_msg);
        }
        v = value;
        break;
      }
      case msgpack::type::NIL:
        v = Optional<T>{};
        break;
      default:
        v = obj.as<T>();
        break;
    }

    return obj;
  }
};

template<class T>
struct pack<Optional<T>> {
  template<typename Stream>
  packer<Stream> &operator()(msgpack::packer<Stream> &packer, const Optional<T> &v) const noexcept {
    switch (v.value_state()) {
      case OptionalState::has_value:
        packer_float32_decorator::pack_value(packer, v.val());
        break;
      case OptionalState::false_value:
        packer.pack_false();
        break;
      case OptionalState::null_value:
        packer.pack_nil();
        break;
      default:
        __builtin_unreachable();
    }

    return packer;
  }
};

} // namespace vk::msgpack::adaptor
