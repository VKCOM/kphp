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

enum class container_type { ARRAY_ITEM, MAP_KEY, MAP_VALUE };

enum class parse_return { SUCCESS = 2, EXTRA_BYTES = 1, CONTINUE = 0, PARSE_ERROR = -1, STOP_VISITOR = -2 };

struct unpack_stack {
  struct stack_elem {
    stack_elem(container_type type, uint32_t rest) noexcept
      : m_type(type)
      , m_rest(rest) {}
    container_type m_type{};
    uint32_t m_rest{0};
  };

  unpack_stack() noexcept;

  template<typename Visitor>
  parse_return push(Visitor &visitor, container_type type, uint32_t rest);

  template<typename Visitor>
  parse_return consume(Visitor &visitor);

private:
  std::vector<stack_elem> m_stack;
};

enum class msgpack_cs : uint32_t;

template<typename Visitor>
class parser {
public:
  static parse_return parse(const char *data, size_t len, size_t &off, Visitor &v);

private:
  explicit parser(Visitor &visitor) noexcept;

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
  msgpack_cs m_cs{};
  unpack_stack m_stack;
};

} // namespace msgpack
