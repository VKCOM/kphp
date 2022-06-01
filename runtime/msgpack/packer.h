//
// MessagePack for C++ serializing routine
//
// Copyright (C) 2008-2016 FURUHASHI Sadayuki and KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <cstdint>
#include <cstddef>

#include "common/mixin/not_copyable.h"

#include "runtime/msgpack/adaptor/adaptor_base.h"

namespace msgpack {

// this class supports packing into a Stream according to msgpack format.
// See https://github.com/msgpack/msgpack/blob/master/spec.md
template<typename Stream>
class packer : private vk::not_copyable {
public:
  explicit packer(Stream &s);

  template<typename T>
  void pack(const T &v) {
    adaptor::pack<T>{}(*this, v);
  }

  packer<Stream> &pack_uint8(uint8_t d);
  packer<Stream> &pack_uint32(uint32_t d);
  packer<Stream> &pack_uint64(uint64_t d);

  packer<Stream> &pack_int32(int32_t d);
  packer<Stream> &pack_int64(int64_t d);

  packer<Stream> &pack_float(float d);
  packer<Stream> &pack_double(double d);

  packer<Stream> &pack_nil();

  packer<Stream> &pack_true();
  packer<Stream> &pack_false();

  packer<Stream> &pack_array(uint32_t n);
  packer<Stream> &pack_map(uint32_t n);

  packer<Stream> &pack_str(uint32_t l);
  packer<Stream> &pack_str_body(const char *b, uint32_t l);

private:
  template<typename T>
  void pack_imp_uint8(T d);
  template<typename T>
  void pack_imp_uint32(T d);
  template<typename T>
  void pack_imp_uint64(T d);
  template<typename T>
  void pack_imp_int32(T d);
  template<typename T>
  void pack_imp_int64(T d);

  void append_buffer(const char *buf, size_t len) {
    m_stream.write(buf, len);
  }

  Stream &m_stream;
};

template<typename Stream, typename T>
void pack(Stream &s, const T &v) {
  packer<Stream>(s).pack(v);
}

} // namespace msgpack
