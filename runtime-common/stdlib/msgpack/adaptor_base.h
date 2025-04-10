// Compiler for PHP (aka KPHP)
// msgpack (c) https://github.com/msgpack/msgpack-c/tree/cpp_master (copied as third-party and slightly modified)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

namespace vk::msgpack {

struct object;

template<typename Stream>
class packer;

namespace adaptor {

template<typename T>
struct convert {
  void operator()(const msgpack::object& o, T& v) const {
    v.msgpack_unpack(o);
  }
};

template<typename T>
struct pack {
  template<typename Stream>
  void operator()(msgpack::packer<Stream>& o, const T& v) const {
    v.msgpack_pack(o);
  }
};

} // namespace adaptor

} // namespace vk::msgpack
