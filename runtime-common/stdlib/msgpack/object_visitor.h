// Compiler for PHP (aka KPHP)
// msgpack (c) https://github.com/msgpack/msgpack-c/tree/cpp_master (copied as third-party and slightly modified)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <vector>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/stdlib/msgpack/object.h"

namespace vk::msgpack {

class zone;

class object_visitor : vk::not_copyable {
public:
  explicit object_visitor(msgpack::zone &zone) noexcept;

  msgpack::object &&flush() &&noexcept {
    return std::move(m_obj);
  }

  bool visit_nil() noexcept;
  bool visit_boolean(bool v) noexcept;
  bool visit_positive_integer(uint64_t v) noexcept;
  bool visit_negative_integer(int64_t v) noexcept;
  bool visit_float32(float v) noexcept;
  bool visit_float64(double v) noexcept;
  bool visit_str(const char *v, uint32_t size);
  bool start_array(uint32_t num_elements);
  bool start_map(uint32_t num_kv_pairs);
  bool start_array_item() const noexcept {
    return true;
  }
  bool end_array_item() noexcept {
    ++m_stack.back();
    return true;
  }
  bool end_array() noexcept {
    m_stack.pop_back();
    return true;
  }
  bool start_map_key() const noexcept {
    return true;
  }
  bool end_map_key() noexcept {
    ++m_stack.back();
    return true;
  }
  bool start_map_value() const noexcept {
    return true;
  }
  bool end_map_value() noexcept {
    ++m_stack.back();
    return true;
  }
  bool end_map() noexcept {
    m_stack.pop_back();
    return true;
  }
  void parse_error(size_t /*parsed_offset*/, size_t /*error_offset*/) const;
  void insufficient_bytes(size_t /*parsed_offset*/, size_t /*error_offset*/) const;

private:
  msgpack::object m_obj;
  kphp::stl::vector<msgpack::object *, kphp::memory::script_allocator> m_stack{};
  msgpack::zone &m_zone;
};

} // namespace vk::msgpack
