// Compiler for PHP (aka KPHP)
// msgpack (c) https://github.com/msgpack/msgpack-c/tree/cpp_master (copied as third-party and slightly modified)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <stdexcept>

#include "runtime/kphp_core.h"
#include "runtime/msgpack/adaptor_base.h"
#include "runtime/msgpack/check_instance_depth.h"
#include "runtime/msgpack/object.h"
#include "runtime/msgpack/packer.h"
#include "runtime/msgpack/unpack_exception.h"

namespace vk::msgpack {

class type_error : public std::exception {};

namespace detail {

template<typename T, bool Signed>
struct convert_integer_sign;

template<typename T>
struct convert_integer_sign<T, true> {
  static T convert(const msgpack::object &o) {
    if (o.type == stored_type::POSITIVE_INTEGER) {
      if (o.via.u64 > static_cast<uint64_t>(std::numeric_limits<T>::max())) {
        throw type_error{};
      }
      return static_cast<T>(o.via.u64);
    } else if (o.type == stored_type::NEGATIVE_INTEGER) {
      if (o.via.i64 < static_cast<int64_t>(std::numeric_limits<T>::min())) {
        throw type_error{};
      }
      return static_cast<T>(o.via.i64);
    }
    throw type_error{};
  }
};

template<typename T>
struct convert_integer_sign<T, false> {
  static T convert(const msgpack::object &o) {
    if (o.type == stored_type::POSITIVE_INTEGER) {
      if (o.via.u64 > static_cast<uint64_t>(std::numeric_limits<T>::max())) {
        throw type_error{};
      }
      return static_cast<T>(o.via.u64);
    }
    throw type_error{};
  }
};

template<typename T>
struct is_signed {
  static const bool value = std::numeric_limits<T>::is_signed;
};

template<typename T>
inline T convert_integer(const msgpack::object &o) {
  return detail::convert_integer_sign<T, is_signed<T>::value>::convert(o);
}

} // namespace detail

namespace adaptor {

template<>
struct convert<int32_t> {
  void operator()(const msgpack::object &o, int32_t &v) const {
    v = detail::convert_integer<int32_t>(o);
  }
};

template<>
struct convert<int64_t> {
  void operator()(const msgpack::object &o, int64_t &v) const {
    v = detail::convert_integer<int64_t>(o);
  }
};

template<>
struct convert<uint8_t> {
  void operator()(const msgpack::object &o, uint8_t &v) const {
    v = detail::convert_integer<uint8_t>(o);
  }
};

template<>
struct convert<uint32_t> {
  void operator()(const msgpack::object &o, uint32_t &v) const {
    v = detail::convert_integer<uint32_t>(o);
  }
};

template<>
struct convert<uint64_t> {
  void operator()(const msgpack::object &o, uint64_t &v) const {
    v = detail::convert_integer<uint64_t>(o);
  }
};

template<>
struct pack<int32_t> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, int32_t v) const {
    o.pack_int32(v);
  }
};

template<>
struct pack<int64_t> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, int64_t v) const {
    o.pack_int64(v);
  }
};

template<>
struct pack<uint8_t> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, uint8_t v) const {
    o.pack_uint8(v);
  }
};

template<>
struct pack<uint32_t> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, uint32_t v) const {
    o.pack_uint32(v);
  }
};

template<>
struct pack<uint64_t> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, uint64_t v) const {
    o.pack_uint64(v);
  }
};

template<>
struct convert<bool> {
  void operator()(const msgpack::object &o, bool &v) const {
    if (o.type != stored_type::BOOLEAN) {
      throw type_error{};
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

template<>
struct convert<float> {
  void operator()(const msgpack::object &o, float &v) const {
    if (o.type == stored_type::FLOAT32 || o.type == stored_type::FLOAT64) {
      v = static_cast<float>(o.via.f64);
    } else if (o.type == stored_type::POSITIVE_INTEGER) {
      v = static_cast<float>(o.via.u64);
    } else if (o.type == stored_type::NEGATIVE_INTEGER) {
      v = static_cast<float>(o.via.i64);
    } else {
      throw type_error{};
    }
  }
};

template<>
struct pack<float> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, const float &v) const {
    o.pack_float(v);
  }
};

template<>
struct convert<double> {
  void operator()(const msgpack::object &o, double &v) const {
    if (o.type == stored_type::FLOAT32 || o.type == stored_type::FLOAT64) {
      v = o.via.f64;
    } else if (o.type == stored_type::POSITIVE_INTEGER) {
      v = static_cast<double>(o.via.u64);
    } else if (o.type == stored_type::NEGATIVE_INTEGER) {
      v = static_cast<double>(o.via.i64);
    } else {
      throw type_error{};
    }
  }
};

template<>
struct pack<double> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, const double &v) const {
    o.pack_double(v);
  }
};

template<class T>
struct convert<array<T>> {
  const msgpack::object &operator()(const msgpack::object &obj, array<T> &res_arr) const {
    if (obj.type == stored_type::ARRAY) {
      res_arr.reserve(obj.via.array.size, true);

      for (uint32_t i = 0; i < obj.via.array.size; ++i) {
        res_arr.set_value(static_cast<int64_t>(i), obj.via.array.ptr[i].as<T>());
      }

      return obj;
    }

    if (obj.type == stored_type::MAP) {
      array_size size(0, false);
      run_callbacks_on_map(
        obj.via.map, [&size](int64_t, msgpack::object &) { size.size++; }, [&size](const string &, msgpack::object &) { size.size++; });

      res_arr.reserve(size.size, size.is_vector);
      run_callbacks_on_map(
        obj.via.map, [&res_arr](int64_t key, msgpack::object &value) { res_arr.set_value(key, value.as<T>()); },
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
        case stored_type::POSITIVE_INTEGER:
        case stored_type::NEGATIVE_INTEGER:
          on_integer(key.as<int64_t>(), value);
          break;
        case stored_type::STR:
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

template<class T>
struct convert<class_instance<T>> {
  const msgpack::object &operator()(const msgpack::object &obj, class_instance<T> &instance) const {
    switch (obj.type) {
      case stored_type::NIL:
        instance = class_instance<T>{};
        break;
      case stored_type::ARRAY:
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

template<>
struct convert<string> {
  const msgpack::object &operator()(const msgpack::object &obj, string &res_s) const {
    if (obj.type != stored_type::STR) {
      throw type_error{};
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

template<typename Tuple, std::size_t N>
struct StdTupleConverter {
  static void convert(const msgpack::object &o, Tuple &v) {
    StdTupleConverter<Tuple, N - 1>::convert(o, v);
    if (o.via.array.size >= N)
      o.via.array.ptr[N - 1].convert<typename std::remove_reference<decltype(std::get<N - 1>(v))>::type>(std::get<N - 1>(v));
  }
};

template<typename Tuple>
struct StdTupleConverter<Tuple, 0> {
  static void convert(const msgpack::object & /*o*/, Tuple & /*v*/) {}
};

template<size_t N>
struct PackValueHelper {
  template<class StreamT, class TupleT>
  static void convert(packer<StreamT> &packer, const TupleT &value) {
    PackValueHelper<N - 1>::convert(packer, value);
    packer_float32_decorator::pack_value(packer, std::get<N - 1>(value));
  }
};

template<>
struct PackValueHelper<0> {
  template<class StreamT, class TupleT>
  static void convert(packer<StreamT> & /*o*/, const TupleT & /*v*/) {}
};

template<typename... Args>
struct convert<std::tuple<Args...>> {
  void operator()(const msgpack::object &o, std::tuple<Args...> &v) const {
    if (o.type != stored_type::ARRAY) {
      throw type_error{};
    }
    StdTupleConverter<decltype(v), sizeof...(Args)>::convert(o, v);
  }
};

template<typename... Args>
struct pack<std::tuple<Args...>> {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, const std::tuple<Args...> &v) const {
    o.pack_array(sizeof...(Args));
    PackValueHelper<sizeof...(Args)>::convert(o, v);
  }
};

template<class T>
struct convert<Optional<T>> {
  const msgpack::object &operator()(const msgpack::object &obj, Optional<T> &v) const {
    switch (obj.type) {
      case stored_type::BOOLEAN: {
        bool value = obj.as<bool>();
        if (!std::is_same<T, bool>{} && value) {
          char err_msg[256];
          snprintf(err_msg, 256, "Expected false for type `%s|false` but true was given", typeid(T).name());
          throw msgpack::unpack_error(err_msg);
        }
        v = value;
        break;
      }
      case stored_type::NIL:
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

template<>
struct convert<mixed> {
  const msgpack::object &operator()(const msgpack::object &obj, mixed &v) const {
    switch (obj.type) {
      case stored_type::STR:
        v = obj.as<string>();
        break;
      case stored_type::ARRAY:
        v = obj.as<array<mixed>>();
        break;
      case stored_type::NEGATIVE_INTEGER:
      case stored_type::POSITIVE_INTEGER:
        v = obj.as<int64_t>();
        break;
      case stored_type::FLOAT32:
      case stored_type::FLOAT64:
        v = obj.as<double>();
        break;
      case stored_type::BOOLEAN:
        v = obj.as<bool>();
        break;
      case stored_type::MAP:
        v = obj.as<array<mixed>>();
        break;
      case stored_type::NIL:
        v = mixed{};
        break;
      default:
        throw type_error{};
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

} // namespace adaptor
} // namespace vk::msgpack
