// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"
#include "runtime/msgpack/adaptor/adaptor_base.h"
#include "runtime/msgpack/object.h"
#include "runtime/msgpack/unpack_exception.h"

namespace msgpack::adaptor {
struct CheckInstanceDepth {
  static size_t depth;
  static constexpr size_t max_depth = 20;

  CheckInstanceDepth() {
    depth++;
  }

  CheckInstanceDepth(const CheckInstanceDepth &) = delete;
  CheckInstanceDepth &operator=(const CheckInstanceDepth &) = delete;

  static bool is_exceeded() {
    return depth > max_depth;
  }

  ~CheckInstanceDepth() {
    if (!is_exceeded()) {
      depth--;
    }
  }
};

template<class T>
struct convert<class_instance<T>> {
  const msgpack::object &operator()(const msgpack::object &obj, class_instance<T> &instance) const {
    switch (obj.type) {
      case msgpack::type::NIL:
        instance = class_instance<T>{};
        break;
      case msgpack::type::ARRAY:
        instance = class_instance<T>{}.alloc();
        obj.convert(*instance.get());
        break;
      default:
        throw msgpack::unpack_error("Expected NIL or ARRAY type for unpacking class_instance");
    }

    return obj;
  }
};

template<class T>
struct pack<class_instance<T>> {
  template<typename Stream>
  packer<Stream> &operator()(msgpack::packer<Stream> &packer, const class_instance<T> &instance) const noexcept {
    CheckInstanceDepth check_depth;
    if (CheckInstanceDepth::is_exceeded()) {
      return packer;
    }
    if (instance.is_null()) {
      packer.pack_nil();
    } else {
      packer.pack(*instance.get());
    }

    return packer;
  }
};

} // namespace msgpack::adaptor
