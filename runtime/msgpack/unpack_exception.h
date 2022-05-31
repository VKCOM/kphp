//
// MessagePack for C++ deserializing routine
//
// Copyright (C) 2008-2016 FURUHASHI Sadayuki and KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <stdexcept>

namespace msgpack {

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

struct str_size_overflow : public size_overflow {
  using size_overflow::size_overflow;
};

struct bin_size_overflow : public size_overflow {
  using size_overflow::size_overflow;
};

struct ext_size_overflow : public size_overflow {
  using size_overflow::size_overflow;
};

struct depth_size_overflow : public size_overflow {
  using size_overflow::size_overflow;
};

} // namespace msgpack
