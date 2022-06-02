// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <stdexcept>

namespace vk::msgpack {

struct object;

template<typename Stream>
class packer;

namespace adaptor {

template<typename T>
struct convert {
  void operator()(const msgpack::object &o, T &v) const {
    v.msgpack_unpack(o);
  }
};

template<typename T>
struct pack {
  template<typename Stream>
  void operator()(msgpack::packer<Stream> &o, const T &v) const {
    v.msgpack_pack(o);
  }
};

} // namespace adaptor

class type_error : public std::bad_cast {};

} // namespace vk::msgpack
