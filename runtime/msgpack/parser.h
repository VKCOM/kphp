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

#include <vector>

namespace msgpack {

enum class msgpack_cs : uint32_t {
  HEADER = 0x00, // nil

  // MSGPACK_CS_                = 0x01,
  // MSGPACK_CS_                = 0x02,  // false
  // MSGPACK_CS_                = 0x03,  // true

  BIN_8 = 0x04,
  BIN_16 = 0x05,
  BIN_32 = 0x06,

  EXT_8 = 0x07,
  EXT_16 = 0x08,
  EXT_32 = 0x09,

  FLOAT = 0x0a,
  DOUBLE = 0x0b,
  UINT_8 = 0x0c,
  UINT_16 = 0x0d,
  UINT_32 = 0x0e,
  UINT_64 = 0x0f,
  INT_8 = 0x10,
  INT_16 = 0x11,
  INT_32 = 0x12,
  INT_64 = 0x13,

  FIXEXT_1 = 0x14,
  FIXEXT_2 = 0x15,
  FIXEXT_4 = 0x16,
  FIXEXT_8 = 0x17,
  FIXEXT_16 = 0x18,

  STR_8 = 0x19,  // str8
  STR_16 = 0x1a, // str16
  STR_32 = 0x1b, // str32
  ARRAY_16 = 0x1c,
  ARRAY_32 = 0x1d,
  MAP_16 = 0x1e,
  MAP_32 = 0x1f,

  // ACS_BIG_INT_VALUE,
  // ACS_BIG_FLOAT_VALUE,
  ACS_STR_VALUE,
  ACS_BIN_VALUE,
  ACS_EXT_VALUE
};

enum msgpack_container_type { MSGPACK_CT_ARRAY_ITEM, MSGPACK_CT_MAP_KEY, MSGPACK_CT_MAP_VALUE };

enum parse_return {
  PARSE_SUCCESS              =  2,
  PARSE_EXTRA_BYTES          =  1,
  PARSE_CONTINUE             =  0,
  PARSE_PARSE_ERROR          = -1,
  PARSE_STOP_VISITOR         = -2
};

struct unpack_stack {
  struct stack_elem {
    stack_elem(msgpack_container_type type, uint32_t rest) noexcept
      : m_type(type)
      , m_rest(rest) {}
    msgpack_container_type m_type{};
    uint32_t m_rest{0};
  };

  unpack_stack() noexcept;

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
  static parse_return parse(const char *data, size_t len, size_t &off, Visitor &v);

private:
  explicit parser(Visitor &visitor) noexcept
    : visitor_(visitor) {}

  parse_return execute(const char *data, std::size_t len, std::size_t &off);

  template<typename T>
  static msgpack_cs next_cs(T p) noexcept;

  template<typename T, typename StartVisitor, typename EndVisitor>
  parse_return start_aggregate(const StartVisitor &sv, const EndVisitor &ev, const char *load_pos, std::size_t &off);

  parse_return after_visit_proc(bool visit_result, std::size_t &off);

  Visitor &visitor_;

  const char *m_start{nullptr};
  const char *m_current{nullptr};

  std::size_t m_trail{0};
  msgpack_cs m_cs{msgpack_cs::HEADER};
  unpack_stack m_stack;
};

} // namespace msgpack
