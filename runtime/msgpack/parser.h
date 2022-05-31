//
// MessagePack for C++ deserializing routine
//
// Copyright (C) 2016-2017 KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <cstddef>
#include <vector>

#include "runtime/msgpack/parse_return.h"
#include "runtime/msgpack/unpack_decl.h"
#include "runtime/msgpack/unpack_exception.h"

namespace msgpack {

enum {
  MSGPACK_CS_HEADER = 0x00, // nil

  // MSGPACK_CS_                = 0x01,
  // MSGPACK_CS_                = 0x02,  // false
  // MSGPACK_CS_                = 0x03,  // true

  MSGPACK_CS_BIN_8 = 0x04,
  MSGPACK_CS_BIN_16 = 0x05,
  MSGPACK_CS_BIN_32 = 0x06,

  MSGPACK_CS_EXT_8 = 0x07,
  MSGPACK_CS_EXT_16 = 0x08,
  MSGPACK_CS_EXT_32 = 0x09,

  MSGPACK_CS_FLOAT = 0x0a,
  MSGPACK_CS_DOUBLE = 0x0b,
  MSGPACK_CS_UINT_8 = 0x0c,
  MSGPACK_CS_UINT_16 = 0x0d,
  MSGPACK_CS_UINT_32 = 0x0e,
  MSGPACK_CS_UINT_64 = 0x0f,
  MSGPACK_CS_INT_8 = 0x10,
  MSGPACK_CS_INT_16 = 0x11,
  MSGPACK_CS_INT_32 = 0x12,
  MSGPACK_CS_INT_64 = 0x13,

  MSGPACK_CS_FIXEXT_1 = 0x14,
  MSGPACK_CS_FIXEXT_2 = 0x15,
  MSGPACK_CS_FIXEXT_4 = 0x16,
  MSGPACK_CS_FIXEXT_8 = 0x17,
  MSGPACK_CS_FIXEXT_16 = 0x18,

  MSGPACK_CS_STR_8 = 0x19,  // str8
  MSGPACK_CS_STR_16 = 0x1a, // str16
  MSGPACK_CS_STR_32 = 0x1b, // str32
  MSGPACK_CS_ARRAY_16 = 0x1c,
  MSGPACK_CS_ARRAY_32 = 0x1d,
  MSGPACK_CS_MAP_16 = 0x1e,
  MSGPACK_CS_MAP_32 = 0x1f,

  // MSGPACK_ACS_BIG_INT_VALUE,
  // MSGPACK_ACS_BIG_FLOAT_VALUE,
  MSGPACK_ACS_STR_VALUE,
  MSGPACK_ACS_BIN_VALUE,
  MSGPACK_ACS_EXT_VALUE
};

enum msgpack_container_type { MSGPACK_CT_ARRAY_ITEM, MSGPACK_CT_MAP_KEY, MSGPACK_CT_MAP_VALUE };

struct unpack_stack {
  struct stack_elem {
    stack_elem(msgpack_container_type type, uint32_t rest) noexcept
      : m_type(type)
      , m_rest(rest) {}
    msgpack_container_type m_type;
    uint32_t m_rest;
  };

  unpack_stack() noexcept {
    m_stack.reserve(MSGPACK_EMBED_STACK_SIZE);
  }

  template<typename Visitor>
  parse_return push(Visitor &visitor, msgpack_container_type type, uint32_t rest);

  template<typename Visitor>
  parse_return consume(Visitor &visitor);

private:
  std::vector<stack_elem> m_stack;
};

template<typename Visitor>
class parser {
public:
  explicit parser(Visitor &visitor) noexcept
    : visitor_(visitor) {}

  parse_return execute(const char *data, std::size_t len, std::size_t &off);

private:
  template<typename T>
  static uint32_t next_cs(T p) {
    return static_cast<uint32_t>(*p) & 0x1f;
  }

  template<typename T, typename StartVisitor, typename EndVisitor>
  parse_return start_aggregate(const StartVisitor &sv, const EndVisitor &ev, const char *load_pos, std::size_t &off);

  parse_return after_visit_proc(bool visit_result, std::size_t &off);

  Visitor &visitor_;

  const char *m_start{nullptr};
  const char *m_current{nullptr};

  std::size_t m_trail{0};
  uint32_t m_cs{MSGPACK_CS_HEADER};
  unpack_stack m_stack;
};

template<typename Visitor>
parse_return parse_imp(const char *data, size_t len, size_t &off, Visitor &v);

} // namespace msgpack
