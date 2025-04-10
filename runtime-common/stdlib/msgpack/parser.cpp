// Compiler for PHP (aka KPHP)
// msgpack (c) https://github.com/msgpack/msgpack-c/tree/cpp_master (copied as third-party and slightly modified)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cassert>
#include <cstring>
#include <type_traits>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/stdlib/msgpack/object_visitor.h"
#include "runtime-common/stdlib/msgpack/parser.h"
#include "runtime-common/stdlib/msgpack/sysdep.h"

namespace vk::msgpack {
namespace {
struct fix_tag {
  char f1[65]; // FIXME unique size is required. or use is_same meta function.
};

template<typename T>
struct value {
  using type = T;
};

template<>
struct value<fix_tag> {
  using type = uint32_t;
};

template<typename T>
std::enable_if_t<sizeof(T) == sizeof(fix_tag)> load(uint32_t& dst, const char* n) noexcept {
  dst = static_cast<uint32_t>(*reinterpret_cast<const uint8_t*>(n)) & 0x0f;
}

template<typename T>
std::enable_if_t<sizeof(T) == 1> load(T& dst, const char* n) noexcept {
  dst = static_cast<T>(*reinterpret_cast<const uint8_t*>(n));
}

template<typename T>
std::enable_if_t<sizeof(T) == 2> load(T& dst, const char* n) noexcept {
  _msgpack_load16(T, n, &dst);
}

template<typename T>
std::enable_if_t<sizeof(T) == 4> load(T& dst, const char* n) noexcept {
  _msgpack_load32(T, n, &dst);
}

template<typename T>
std::enable_if_t<sizeof(T) == 8> load(T& dst, const char* n) noexcept {
  _msgpack_load64(T, n, &dst);
}

enum class container_type { ARRAY_ITEM, MAP_KEY, MAP_VALUE };

template<typename Visitor>
struct array_sv {
  explicit array_sv(Visitor& visitor) noexcept
      : visitor_(visitor) {}
  bool operator()(uint32_t size) const {
    return visitor_.start_array(size);
  }
  container_type type() const {
    return container_type::ARRAY_ITEM;
  }

private:
  Visitor& visitor_;
};

template<typename Visitor>
struct array_ev {
  explicit array_ev(Visitor& visitor) noexcept
      : visitor_(visitor) {}
  bool operator()() const {
    return visitor_.end_array();
  }

private:
  Visitor& visitor_;
};

template<typename Visitor>
struct map_sv {
  explicit map_sv(Visitor& visitor) noexcept
      : visitor_(visitor) {}
  bool operator()(uint32_t size) const {
    return visitor_.start_map(size);
  }
  container_type type() const {
    return container_type::MAP_KEY;
  }

private:
  Visitor& visitor_;
};

template<typename Visitor>
struct map_ev {
  explicit map_ev(Visitor& visitor) noexcept
      : visitor_(visitor) {}
  bool operator()() const {
    return visitor_.end_map();
  }

private:
  Visitor& visitor_;
};
} // namespace

struct unpack_stack {
  struct stack_elem {
    stack_elem(container_type type, uint32_t rest) noexcept
        : m_type(type),
          m_rest(rest) {}
    container_type m_type{};
    uint32_t m_rest{0};
  };

  unpack_stack() noexcept;

  template<typename Visitor>
  parse_return push(Visitor& visitor, container_type type, uint32_t rest);

  template<typename Visitor>
  parse_return consume(Visitor& visitor);

private:
  kphp::stl::vector<stack_elem, kphp::memory::script_allocator> m_stack;
};

unpack_stack::unpack_stack() noexcept {
  constexpr static std::size_t STACK_SIZE = 32;
  m_stack.reserve(STACK_SIZE);
}

template<typename Visitor>
parse_return unpack_stack::push(Visitor& visitor, container_type type, uint32_t rest) {
  m_stack.emplace_back(type, rest);
  switch (type) {
  case container_type::ARRAY_ITEM:
    return visitor.start_array_item() ? parse_return::CONTINUE : parse_return::STOP_VISITOR;
  case container_type::MAP_KEY:
    return visitor.start_map_key() ? parse_return::CONTINUE : parse_return::STOP_VISITOR;
  case container_type::MAP_VALUE:
    assert(0);
    return parse_return::STOP_VISITOR;
  }
  assert(0);
  return parse_return::STOP_VISITOR;
}

template<typename Visitor>
parse_return unpack_stack::consume(Visitor& visitor) {
  while (!m_stack.empty()) {
    stack_elem& e = m_stack.back();
    switch (e.m_type) {
    case container_type::ARRAY_ITEM:
      if (!visitor.end_array_item())
        return parse_return::STOP_VISITOR;
      if (--e.m_rest == 0) {
        m_stack.pop_back();
        if (!visitor.end_array())
          return parse_return::STOP_VISITOR;
      } else {
        if (!visitor.start_array_item())
          return parse_return::STOP_VISITOR;
        return parse_return::CONTINUE;
      }
      break;
    case container_type::MAP_KEY:
      if (!visitor.end_map_key())
        return parse_return::STOP_VISITOR;
      if (!visitor.start_map_value())
        return parse_return::STOP_VISITOR;
      e.m_type = container_type::MAP_VALUE;
      return parse_return::CONTINUE;
    case container_type::MAP_VALUE:
      if (!visitor.end_map_value())
        return parse_return::STOP_VISITOR;
      if (--e.m_rest == 0) {
        m_stack.pop_back();
        if (!visitor.end_map())
          return parse_return::STOP_VISITOR;
      } else {
        e.m_type = container_type::MAP_KEY;
        if (!visitor.start_map_key())
          return parse_return::STOP_VISITOR;
        return parse_return::CONTINUE;
      }
      break;
    }
  }
  return parse_return::SUCCESS;
}

enum class msgpack_cs : uint32_t {
  HEADER = 0x00, // nil

  // MSGPACK_CS_ = 0x01,
  // MSGPACK_CS_ = 0x02,  // false
  // MSGPACK_CS_ = 0x03,  // true

  //  bin and ext support is removed
  //
  //  BIN_8 = 0x04,
  //  BIN_16 = 0x05,
  //  BIN_32 = 0x06,
  //
  //  EXT_8 = 0x07,
  //  EXT_16 = 0x08,
  //  EXT_32 = 0x09,

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

  //  ext support is removed
  //
  //  FIXEXT_1 = 0x14,
  //  FIXEXT_2 = 0x15,
  //  FIXEXT_4 = 0x16,
  //  FIXEXT_8 = 0x17,
  //  FIXEXT_16 = 0x18,

  STR_8 = 0x19,  // str8
  STR_16 = 0x1a, // str16
  STR_32 = 0x1b, // str32
  ARRAY_16 = 0x1c,
  ARRAY_32 = 0x1d,
  MAP_16 = 0x1e,
  MAP_32 = 0x1f,

  // ACS_BIG_INT_VALUE,
  // ACS_BIG_FLOAT_VALUE,
  ACS_STR_VALUE
};

template<typename Visitor>
template<typename T, typename StartVisitor, typename EndVisitor>
parse_return parser<Visitor>::start_aggregate(const StartVisitor& sv, const EndVisitor& ev, const char* load_pos, std::size_t& off) {
  typename value<T>::type size;
  load<T>(size, load_pos);
  ++m_current;
  if (size == 0) {
    if (!sv(size)) {
      off = m_current - m_start;
      return parse_return::STOP_VISITOR;
    }
    if (!ev()) {
      off = m_current - m_start;
      return parse_return::STOP_VISITOR;
    }
    parse_return ret = stack_.consume(visitor_);
    if (ret != parse_return::CONTINUE) {
      off = m_current - m_start;
      return ret;
    }
  } else {
    if (!sv(size)) {
      off = m_current - m_start;
      return parse_return::STOP_VISITOR;
    }
    parse_return ret = stack_.push(visitor_, sv.type(), static_cast<uint32_t>(size));
    if (ret != parse_return::CONTINUE) {
      off = m_current - m_start;
      return ret;
    }
  }
  m_cs = msgpack_cs::HEADER;
  return parse_return::CONTINUE;
}

template<typename Visitor>
parse_return parser<Visitor>::after_visit_proc(bool visit_result, std::size_t& off) {
  ++m_current;
  if (!visit_result) {
    off = m_current - m_start;
    return parse_return::STOP_VISITOR;
  }
  parse_return ret = stack_.consume(visitor_);
  if (ret != parse_return::CONTINUE) {
    off = m_current - m_start;
  }
  m_cs = msgpack_cs::HEADER;
  return ret;
}

template<typename Visitor>
template<typename T>
msgpack_cs parser<Visitor>::next_cs(T p) noexcept {
  return msgpack_cs{static_cast<uint32_t>(*p) & 0x1f};
}

template<typename Visitor>
parse_return parser<Visitor>::execute(const char* data, std::size_t len, std::size_t& off) {
  assert(len >= off);

  m_start = data;
  m_current = data + off;
  const char* const pe = data + len;
  const char* n = nullptr;

  if (m_current == pe) {
    off = m_current - m_start;
    return parse_return::CONTINUE;
  }
  bool fixed_trail_again = false;
  do {
    if (m_cs == msgpack_cs::HEADER) {
      fixed_trail_again = false;
      int selector = *reinterpret_cast<const unsigned char*>(m_current);
      if (0x00 <= selector && selector <= 0x7f) { // Positive Fixnum
        uint8_t tmp = *reinterpret_cast<const uint8_t*>(m_current);
        bool visret = visitor_.visit_positive_integer(tmp);
        parse_return upr = after_visit_proc(visret, off);
        if (upr != parse_return::CONTINUE)
          return upr;
      } else if (0xe0 <= selector && selector <= 0xff) { // Negative Fixnum
        int8_t tmp = *reinterpret_cast<const int8_t*>(m_current);
        bool visret = visitor_.visit_negative_integer(tmp);
        parse_return upr = after_visit_proc(visret, off);
        if (upr != parse_return::CONTINUE)
          return upr;
      } else if (0xc4 <= selector && selector <= 0xdf) {
        const uint32_t trail[] = {
            1,  // bin     8  0xc4
            2,  // bin    16  0xc5
            4,  // bin    32  0xc6
            1,  // ext     8  0xc7
            2,  // ext    16  0xc8
            4,  // ext    32  0xc9
            4,  // float  32  0xca
            8,  // float  64  0xcb
            1,  // uint    8  0xcc
            2,  // uint   16  0xcd
            4,  // uint   32  0xce
            8,  // uint   64  0xcf
            1,  // int     8  0xd0
            2,  // int    16  0xd1
            4,  // int    32  0xd2
            8,  // int    64  0xd3
            2,  // fixext  1  0xd4
            3,  // fixext  2  0xd5
            5,  // fixext  4  0xd6
            9,  // fixext  8  0xd7
            17, // fixext 16  0xd8
            1,  // str     8  0xd9
            2,  // str    16  0xda
            4,  // str    32  0xdb
            2,  // array  16  0xdc
            4,  // array  32  0xdd
            2,  // map    16  0xde
            4,  // map    32  0xdf
        };
        m_trail = trail[selector - 0xc4];
        m_cs = next_cs(m_current);
        fixed_trail_again = true;
      } else if (0xa0 <= selector && selector <= 0xbf) { // FixStr
        m_trail = static_cast<uint32_t>(*m_current) & 0x1f;
        if (m_trail == 0) {
          bool visret = visitor_.visit_str(n, static_cast<uint32_t>(m_trail));
          parse_return upr = after_visit_proc(visret, off);
          if (upr != parse_return::CONTINUE)
            return upr;
        } else {
          m_cs = msgpack_cs::ACS_STR_VALUE;
          fixed_trail_again = true;
        }
      } else if (0x90 <= selector && selector <= 0x9f) { // FixArray
        parse_return ret = start_aggregate<fix_tag>(array_sv(visitor_), array_ev(visitor_), m_current, off);
        if (ret != parse_return::CONTINUE)
          return ret;
      } else if (0x80 <= selector && selector <= 0x8f) { // FixMap
        parse_return ret = start_aggregate<fix_tag>(map_sv(visitor_), map_ev(visitor_), m_current, off);
        if (ret != parse_return::CONTINUE)
          return ret;
      } else if (selector == 0xc2) { // false
        bool visret = visitor_.visit_boolean(false);
        parse_return upr = after_visit_proc(visret, off);
        if (upr != parse_return::CONTINUE)
          return upr;
      } else if (selector == 0xc3) { // true
        bool visret = visitor_.visit_boolean(true);
        parse_return upr = after_visit_proc(visret, off);
        if (upr != parse_return::CONTINUE)
          return upr;
      } else if (selector == 0xc0) { // nil
        bool visret = visitor_.visit_nil();
        parse_return upr = after_visit_proc(visret, off);
        if (upr != parse_return::CONTINUE)
          return upr;
      } else {
        off = m_current - m_start;
        visitor_.parse_error(off - 1, off);
        return parse_return::PARSE_ERROR;
      }
      // end msgpack_cs::HEADER
    }
    if (m_cs != msgpack_cs::HEADER || fixed_trail_again) {
      if (fixed_trail_again) {
        ++m_current;
        fixed_trail_again = false;
      }
      if (static_cast<std::size_t>(pe - m_current) < m_trail) {
        off = m_current - m_start;
        return parse_return::CONTINUE;
      }
      n = m_current;
      m_current += m_trail - 1;
      switch (m_cs) {
      case msgpack_cs::FLOAT: {
        union {
          uint32_t i;
          float f;
        } mem;
        load<uint32_t>(mem.i, n);
        bool visret = visitor_.visit_float32(mem.f);
        parse_return upr = after_visit_proc(visret, off);
        if (upr != parse_return::CONTINUE)
          return upr;
      } break;
      case msgpack_cs::DOUBLE: {
        union {
          uint64_t i;
          double f;
        } mem;
        load<uint64_t>(mem.i, n);
#if defined(TARGET_OS_IPHONE)
        // ok
#elif defined(__arm__) && !(__ARM_EABI__) // arm-oabi
        // https://github.com/msgpack/msgpack-perl/pull/1
        mem.i = (mem.i & 0xFFFFFFFFUL) << 32UL | (mem.i >> 32UL);
#endif
        bool visret = visitor_.visit_float64(mem.f);
        parse_return upr = after_visit_proc(visret, off);
        if (upr != parse_return::CONTINUE)
          return upr;
      } break;
      case msgpack_cs::UINT_8: {
        uint8_t tmp;
        load<uint8_t>(tmp, n);
        bool visret = visitor_.visit_positive_integer(tmp);
        parse_return upr = after_visit_proc(visret, off);
        if (upr != parse_return::CONTINUE)
          return upr;
      } break;
      case msgpack_cs::UINT_16: {
        uint16_t tmp;
        load<uint16_t>(tmp, n);
        bool visret = visitor_.visit_positive_integer(tmp);
        parse_return upr = after_visit_proc(visret, off);
        if (upr != parse_return::CONTINUE)
          return upr;
      } break;
      case msgpack_cs::UINT_32: {
        uint32_t tmp;
        load<uint32_t>(tmp, n);
        bool visret = visitor_.visit_positive_integer(tmp);
        parse_return upr = after_visit_proc(visret, off);
        if (upr != parse_return::CONTINUE)
          return upr;
      } break;
      case msgpack_cs::UINT_64: {
        uint64_t tmp;
        load<uint64_t>(tmp, n);
        bool visret = visitor_.visit_positive_integer(tmp);
        parse_return upr = after_visit_proc(visret, off);
        if (upr != parse_return::CONTINUE)
          return upr;
      } break;
      case msgpack_cs::INT_8: {
        int8_t tmp;
        load<int8_t>(tmp, n);
        bool visret = visitor_.visit_negative_integer(tmp);
        parse_return upr = after_visit_proc(visret, off);
        if (upr != parse_return::CONTINUE)
          return upr;
      } break;
      case msgpack_cs::INT_16: {
        int16_t tmp;
        load<int16_t>(tmp, n);
        bool visret = visitor_.visit_negative_integer(tmp);
        parse_return upr = after_visit_proc(visret, off);
        if (upr != parse_return::CONTINUE)
          return upr;
      } break;
      case msgpack_cs::INT_32: {
        int32_t tmp;
        load<int32_t>(tmp, n);
        bool visret = visitor_.visit_negative_integer(tmp);
        parse_return upr = after_visit_proc(visret, off);
        if (upr != parse_return::CONTINUE)
          return upr;
      } break;
      case msgpack_cs::INT_64: {
        int64_t tmp;
        load<int64_t>(tmp, n);
        bool visret = visitor_.visit_negative_integer(tmp);
        parse_return upr = after_visit_proc(visret, off);
        if (upr != parse_return::CONTINUE)
          return upr;
      } break;
      case msgpack_cs::STR_8: {
        uint8_t tmp;
        load<uint8_t>(tmp, n);
        m_trail = tmp;
        if (m_trail == 0) {
          bool visret = visitor_.visit_str(n, static_cast<uint32_t>(m_trail));
          parse_return upr = after_visit_proc(visret, off);
          if (upr != parse_return::CONTINUE)
            return upr;
        } else {
          m_cs = msgpack_cs::ACS_STR_VALUE;
          fixed_trail_again = true;
        }
      } break;
      case msgpack_cs::STR_16: {
        uint16_t tmp;
        load<uint16_t>(tmp, n);
        m_trail = tmp;
        if (m_trail == 0) {
          bool visret = visitor_.visit_str(n, static_cast<uint32_t>(m_trail));
          parse_return upr = after_visit_proc(visret, off);
          if (upr != parse_return::CONTINUE)
            return upr;
        } else {
          m_cs = msgpack_cs::ACS_STR_VALUE;
          fixed_trail_again = true;
        }
      } break;
      case msgpack_cs::STR_32: {
        uint32_t tmp;
        load<uint32_t>(tmp, n);
        m_trail = tmp;
        if (m_trail == 0) {
          bool visret = visitor_.visit_str(n, static_cast<uint32_t>(m_trail));
          parse_return upr = after_visit_proc(visret, off);
          if (upr != parse_return::CONTINUE)
            return upr;
        } else {
          m_cs = msgpack_cs::ACS_STR_VALUE;
          fixed_trail_again = true;
        }
      } break;
      case msgpack_cs::ACS_STR_VALUE: {
        bool visret = visitor_.visit_str(n, static_cast<uint32_t>(m_trail));
        parse_return upr = after_visit_proc(visret, off);
        if (upr != parse_return::CONTINUE)
          return upr;
      } break;
      case msgpack_cs::ARRAY_16: {
        parse_return ret = start_aggregate<uint16_t>(array_sv(visitor_), array_ev(visitor_), n, off);
        if (ret != parse_return::CONTINUE)
          return ret;

      } break;
      case msgpack_cs::ARRAY_32: {
        parse_return ret = start_aggregate<uint32_t>(array_sv(visitor_), array_ev(visitor_), n, off);
        if (ret != parse_return::CONTINUE)
          return ret;
      } break;
      case msgpack_cs::MAP_16: {
        parse_return ret = start_aggregate<uint16_t>(map_sv(visitor_), map_ev(visitor_), n, off);
        if (ret != parse_return::CONTINUE)
          return ret;
      } break;
      case msgpack_cs::MAP_32: {
        parse_return ret = start_aggregate<uint32_t>(map_sv(visitor_), map_ev(visitor_), n, off);
        if (ret != parse_return::CONTINUE)
          return ret;
      } break;
      default:
        off = m_current - m_start;
        visitor_.parse_error(n - m_start - 1, n - m_start);
        return parse_return::PARSE_ERROR;
      }
    }
  } while (m_current != pe);

  off = m_current - m_start;
  return parse_return::CONTINUE;
}

template<typename Visitor>
parser<Visitor>::parser(Visitor& visitor, unpack_stack& stack) noexcept
    : visitor_(visitor),
      stack_(stack),
      m_cs(msgpack_cs::HEADER) {}

template<typename Visitor>
parse_return parser<Visitor>::parse(const char* data, size_t len, size_t& off, Visitor& v) {
  std::size_t noff = off;

  if (len <= noff) {
    // FIXME
    v.insufficient_bytes(noff, noff);
    return parse_return::CONTINUE;
  }
  unpack_stack stack;
  parser parse{v, stack};
  parse_return ret = parse.execute(data, len, noff);
  switch (ret) {
  case parse_return::CONTINUE:
    off = noff;
    v.insufficient_bytes(noff - 1, noff);
    return ret;
  case parse_return::SUCCESS:
    off = noff;
    if (noff < len) {
      return parse_return::EXTRA_BYTES;
    }
    return ret;
  default:
    return ret;
  }
}

template parse_return parser<object_visitor>::parse(const char* data, size_t len, size_t& off, object_visitor& v);
} // namespace vk::msgpack
