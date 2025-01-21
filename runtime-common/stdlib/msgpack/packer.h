// Compiler for PHP (aka KPHP)
// msgpack (c) https://github.com/msgpack/msgpack-c/tree/cpp_master (copied as third-party and slightly modified)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>

#include "common/mixin/not_copyable.h"

#include "runtime-common/stdlib/msgpack/adaptor_base.h"
#include "runtime-common/stdlib/serialization/serialization-context.h"

namespace vk::msgpack {

// this class supports packing into a Stream according to msgpack format.
// See https://github.com/msgpack/msgpack/blob/master/spec.md
template<typename Stream>
class packer : private vk::not_copyable {
public:
  explicit packer(Stream &s) noexcept
    : stream_(s) {}

  template<typename T>
  void pack(const T &v) {
    adaptor::pack<T>{}(*this, v);
  }

  void pack_uint8(uint8_t d) noexcept;
  void pack_uint32(uint32_t d) noexcept;
  void pack_uint64(uint64_t d) noexcept;

  void pack_int32(int32_t d) noexcept;
  void pack_int64(int64_t d) noexcept;

  void pack_float(float d) noexcept;
  void pack_double(double d) noexcept;

  void pack_nil() noexcept;

  void pack_true() noexcept;
  void pack_false() noexcept;

  void pack_array(uint32_t n) noexcept;
  void pack_map(uint32_t n) noexcept;

  void pack_str(uint32_t l) noexcept;
  void pack_str_body(const char *b, uint32_t l) noexcept;

private:
  template<typename T>
  void pack_imp_uint8(T d) noexcept;
  template<typename T>
  void pack_imp_uint32(T d) noexcept;
  template<typename T>
  void pack_imp_uint64(T d) noexcept;
  template<typename T>
  void pack_imp_int32(T d) noexcept;
  template<typename T>
  void pack_imp_int64(T d) noexcept;

  void append_buffer(const char *buf, size_t len) noexcept;

  Stream &stream_;
};

class packer_float32_decorator {
public:
  static void clear() noexcept {
    SerializationLibContext::get().serialize_as_float32_ = 0;
  }

  template<class StreamT, class T>
  static void pack_value_float32(packer<StreamT> &packer, const T &value) {
    auto &serialization_context{SerializationLibContext::get()};
    ++serialization_context.serialize_as_float32_;
    pack_value(packer, value);
    --serialization_context.serialize_as_float32_;
  }

  template<class StreamT, class T>
  static void pack_value(packer<StreamT> &packer, const T &value) {
    packer.pack(value);
  }

  template<class StreamT>
  static void pack_value(packer<StreamT> &packer, double value) {
    if (SerializationLibContext::get().serialize_as_float32_ > 0) {
      packer.pack(static_cast<float>(value));
    } else {
      packer.pack(value);
    }
  }
};

} // namespace vk::msgpack
