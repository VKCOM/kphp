// Compiler for PHP (aka KPHP)
// msgpack (c) https://github.com/msgpack/msgpack-c/tree/cpp_master (copied as third-party and slightly modified)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

namespace vk::msgpack {

enum class parse_return { SUCCESS = 2, EXTRA_BYTES = 1, CONTINUE = 0, PARSE_ERROR = -1, STOP_VISITOR = -2 };

struct unpack_stack;

enum class msgpack_cs : uint32_t;

template<typename Visitor>
class parser {
public:
  static parse_return parse(const char *data, size_t len, size_t &off, Visitor &v);

private:
  explicit parser(Visitor &visitor, unpack_stack &stack) noexcept;

  parse_return execute(const char *data, std::size_t len, std::size_t &off);

  template<typename T>
  static msgpack_cs next_cs(T p) noexcept;

  template<typename T, typename StartVisitor, typename EndVisitor>
  parse_return start_aggregate(const StartVisitor &sv, const EndVisitor &ev, const char *load_pos, std::size_t &off);

  parse_return after_visit_proc(bool visit_result, std::size_t &off);

  Visitor &visitor_;
  unpack_stack &stack_;

  const char *m_start{nullptr};
  const char *m_current{nullptr};

  std::size_t m_trail{0};
  msgpack_cs m_cs{};
};

} // namespace vk::msgpack
