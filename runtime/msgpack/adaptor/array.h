// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"
#include "runtime/msgpack/adaptor/adaptor_base.h"
#include "runtime/msgpack/object.h"
#include "runtime/msgpack/packer.h"
#include "runtime/msgpack/unpack_exception.h"

namespace vk::msgpack::adaptor {

template<class T>
struct convert<array<T>> {
  const msgpack::object &operator()(const msgpack::object &obj, array<T> &res_arr) const {
    if (obj.type == msgpack::type::ARRAY) {
      res_arr.reserve(obj.via.array.size, 0, true);

      for (uint32_t i = 0; i < obj.via.array.size; ++i) {
        res_arr.set_value(static_cast<int64_t>(i), obj.via.array.ptr[i].as<T>());
      }

      return obj;
    }

    if (obj.type == msgpack::type::MAP) {
      array_size size(0, 0, false);
      run_callbacks_on_map(obj.via.map,
        [&size](int64_t, msgpack::object &) { size.int_size++; },
        [&size](const string &, msgpack::object &) { size.string_size++; });

      res_arr.reserve(size.int_size, size.string_size, size.is_vector);
      run_callbacks_on_map(obj.via.map,
        [&res_arr](int64_t key, msgpack::object &value) { res_arr.set_value(key, value.as<T>()); },
        [&res_arr](const string &key, msgpack::object &value) { res_arr.set_value(key, value.as<T>()); });

      return obj;
    }

    throw msgpack::unpack_error("couldn't recognize type of unpacking array");
  }

private:
  template<class IntCallbackT, class StrCallbackT>
  static void run_callbacks_on_map(const msgpack::object_map &obj_map, const IntCallbackT &on_integer, const StrCallbackT &on_string) {
    for (size_t i = 0; i < obj_map.size; ++i) {
      auto &key = obj_map.ptr[i].key;
      auto &value = obj_map.ptr[i].val;

      switch (key.type) {
        case msgpack::type::POSITIVE_INTEGER:
        case msgpack::type::NEGATIVE_INTEGER:
          on_integer(key.as<int64_t>(), value);
          break;
        case msgpack::type::STR:
          on_string(key.as<string>(), value);
          break;
        default:
          throw msgpack::unpack_error("expected string or integer in array unpacking");
      }
    }
  }
};

template<class T>
struct pack<array<T>> {
  template<typename Stream>
  packer<Stream> &operator()(msgpack::packer<Stream> &packer, const array<T> &arr) const noexcept {
    if (arr.is_vector() || arr.is_pseudo_vector()) {
      packer.pack_array(static_cast<uint32_t>(arr.count()));
      for (const auto &it : arr) {
        packer_float32_decorator::pack_value(packer, it.get_value());
      }
      return packer;
    }

    packer.pack_map(static_cast<uint32_t>(arr.count()));
    for (const auto &it : arr) {
      if (it.is_string_key()) {
        packer.pack(it.get_string_key());
      } else {
        packer.pack(it.get_int_key());
      }
      packer_float32_decorator::pack_value(packer, it.get_value());
    }

    return packer;
  }
};

} // namespace vk::msgpack::adaptor
