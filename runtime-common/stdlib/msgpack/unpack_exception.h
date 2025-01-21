// Compiler for PHP (aka KPHP)
// msgpack (c) https://github.com/msgpack/msgpack-c/tree/cpp_master (copied as third-party and slightly modified)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <stdexcept>

namespace vk::msgpack {

struct unpack_error : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct parse_error : public unpack_error {
  using unpack_error::unpack_error;
};

struct insufficient_bytes : public unpack_error {
  using unpack_error::unpack_error;
};

struct size_overflow : public unpack_error {
  using unpack_error::unpack_error;
};

struct array_size_overflow : public size_overflow {
  using size_overflow::size_overflow;
};

struct map_size_overflow : public size_overflow {
  using size_overflow::size_overflow;
};

struct ext_size_overflow : public size_overflow {
  using size_overflow::size_overflow;
};

} // namespace vk::msgpack
