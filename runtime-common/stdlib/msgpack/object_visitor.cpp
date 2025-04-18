// Compiler for PHP (aka KPHP)
// msgpack (c) https://github.com/msgpack/msgpack-c/tree/cpp_master (copied as third-party and slightly modified)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstring>

#include "runtime-common/stdlib/msgpack/object_visitor.h"

#include "runtime-common/stdlib/msgpack/unpack_exception.h"
#include "runtime-common/stdlib/msgpack/zone.h"

namespace vk::msgpack {

constexpr static std::size_t STACK_SIZE = 32;

object_visitor::object_visitor(msgpack::zone& zone) noexcept
    : m_zone(zone) {
  m_stack.reserve(STACK_SIZE);
  m_stack.push_back(&m_obj);
}

bool object_visitor::visit_nil() noexcept {
  auto* obj = m_stack.back();
  obj->type = stored_type::NIL;
  return true;
}

bool object_visitor::visit_boolean(bool v) noexcept {
  auto* obj = m_stack.back();
  obj->type = stored_type::BOOLEAN;
  obj->via.boolean = v;
  return true;
}

bool object_visitor::visit_positive_integer(uint64_t v) noexcept {
  auto* obj = m_stack.back();
  obj->type = stored_type::POSITIVE_INTEGER;
  obj->via.u64 = v;
  return true;
}

bool object_visitor::visit_negative_integer(int64_t v) noexcept {
  auto* obj = m_stack.back();
  if (v >= 0) {
    obj->type = stored_type::POSITIVE_INTEGER;
    obj->via.u64 = v;
  } else {
    obj->type = stored_type::NEGATIVE_INTEGER;
    obj->via.i64 = v;
  }
  return true;
}

bool object_visitor::visit_float32(float v) noexcept {
  auto* obj = m_stack.back();
  obj->type = stored_type::FLOAT32;
  obj->via.f64 = v;
  return true;
}

bool object_visitor::visit_float64(double v) noexcept {
  auto* obj = m_stack.back();
  obj->type = stored_type::FLOAT64;
  obj->via.f64 = v;
  return true;
}

bool object_visitor::visit_str(const char* v, uint32_t size) {
  auto* obj = m_stack.back();
  obj->type = stored_type::STR;
  if (v) {
    char* tmp = static_cast<char*>(m_zone.allocate_align(size, alignof(char)));
    if (m_zone.has_error()) {
      error = "cannot allocate";
      return false;
    }
    std::memcpy(tmp, v, size);
    obj->via.str.ptr = tmp;
    obj->via.str.size = size;
  } else {
    obj->via.str.ptr = nullptr;
    obj->via.str.size = 0;
  }
  return true;
}

bool object_visitor::start_array(uint32_t num_elements) {
  auto* obj = m_stack.back();
  obj->type = stored_type::ARRAY;
  obj->via.array.size = num_elements;
  if (num_elements == 0) {
    obj->via.array.ptr = nullptr;
  } else {
    size_t size = num_elements * sizeof(msgpack::object);
    if (size / sizeof(msgpack::object) != num_elements) {
      error = "array size overflow";
      return false;
      // throw msgpack::array_size_overflow("array size overflow");
    }
    auto* ptr = m_zone.allocate_align(size, alignof(msgpack::object));
    if (m_zone.has_error()) {
      error = "cannot allocate";
      return false;
    }
    obj->via.array.ptr = static_cast<msgpack::object*>(ptr);
  }
  m_stack.push_back(obj->via.array.ptr);
  return true;
}

bool object_visitor::start_map(uint32_t num_kv_pairs) {
  auto* obj = m_stack.back();
  obj->type = stored_type::MAP;
  obj->via.map.size = num_kv_pairs;
  if (num_kv_pairs == 0) {
    obj->via.map.ptr = nullptr;
  } else {
    size_t size = num_kv_pairs * sizeof(msgpack::object_kv);
    if (size / sizeof(msgpack::object_kv) != num_kv_pairs) {
      error = "map size overflow";
      return false;
      // throw msgpack::map_size_overflow("map size overflow");
    }
    auto* ptr = m_zone.allocate_align(size, alignof(msgpack::object_kv));
    if (m_zone.has_error()) {
      error = "cannot allocate";
      return false;
    }
    obj->via.map.ptr = static_cast<msgpack::object_kv*>(ptr);
  }
  m_stack.push_back(reinterpret_cast<msgpack::object*>(obj->via.map.ptr));
  return true;
}

void object_visitor::parse_error(size_t /*parsed_offset*/, size_t /*error_offset*/) const {
  error = "parse error";
  // throw msgpack::parse_error("parse error");
}
void object_visitor::insufficient_bytes(size_t /*parsed_offset*/, size_t /*error_offset*/) const {
  error = "insufficient bytes";
  // throw msgpack::insufficient_bytes("insufficient bytes");
}

} // namespace vk::msgpack
