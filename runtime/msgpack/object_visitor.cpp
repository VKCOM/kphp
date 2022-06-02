//
// MessagePack for C++ deserializing routine
//
// Copyright (C) 2017 KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstring>

#include "runtime/msgpack/object_visitor.h"

#include "runtime/msgpack/unpack_exception.h"
#include "runtime/msgpack/zone.h"

namespace msgpack {

constexpr static std::size_t STACK_SIZE = 32;

object_visitor::object_visitor(msgpack::zone &zone) noexcept
  : m_zone(zone) {
  m_stack.reserve(STACK_SIZE);
  m_stack.push_back(&m_obj);
}

bool object_visitor::visit_nil() noexcept {
  auto *obj = m_stack.back();
  obj->type = msgpack::type::NIL;
  return true;
}

bool object_visitor::visit_boolean(bool v) noexcept {
  auto *obj = m_stack.back();
  obj->type = msgpack::type::BOOLEAN;
  obj->via.boolean = v;
  return true;
}

bool object_visitor::visit_positive_integer(uint64_t v) noexcept {
  auto *obj = m_stack.back();
  obj->type = msgpack::type::POSITIVE_INTEGER;
  obj->via.u64 = v;
  return true;
}

bool object_visitor::visit_negative_integer(int64_t v) noexcept {
  auto *obj = m_stack.back();
  if (v >= 0) {
    obj->type = msgpack::type::POSITIVE_INTEGER;
    obj->via.u64 = v;
  } else {
    obj->type = msgpack::type::NEGATIVE_INTEGER;
    obj->via.i64 = v;
  }
  return true;
}

bool object_visitor::visit_float32(float v) noexcept {
  auto *obj = m_stack.back();
  obj->type = msgpack::type::FLOAT32;
  obj->via.f64 = v;
  return true;
}

bool object_visitor::visit_float64(double v) noexcept {
  auto *obj = m_stack.back();
  obj->type = msgpack::type::FLOAT64;
  obj->via.f64 = v;
  return true;
}

bool object_visitor::visit_str(const char *v, uint32_t size) {
  auto *obj = m_stack.back();
  obj->type = msgpack::type::STR;
  char *tmp = static_cast<char *>(m_zone.allocate_align(size, alignof(char)));
  std::memcpy(tmp, v, size);
  obj->via.str.ptr = tmp;

  obj->via.str.size = size;
  return true;
}

bool object_visitor::start_array(uint32_t num_elements) {
  auto *obj = m_stack.back();
  obj->type = msgpack::type::ARRAY;
  obj->via.array.size = num_elements;
  if (num_elements == 0) {
    obj->via.array.ptr = nullptr;
  } else {
    size_t size = num_elements * sizeof(msgpack::object);
    if (size / sizeof(msgpack::object) != num_elements) {
      throw msgpack::array_size_overflow("array size overflow");
    }
    obj->via.array.ptr = static_cast<msgpack::object *>(m_zone.allocate_align(size, alignof(msgpack::object)));
  }
  m_stack.push_back(obj->via.array.ptr);
  return true;
}

bool object_visitor::start_map(uint32_t num_kv_pairs) {
  auto *obj = m_stack.back();
  obj->type = msgpack::type::MAP;
  obj->via.map.size = num_kv_pairs;
  if (num_kv_pairs == 0) {
    obj->via.map.ptr = nullptr;
  } else {
    size_t size = num_kv_pairs * sizeof(msgpack::object_kv);
    if (size / sizeof(msgpack::object_kv) != num_kv_pairs) {
      throw msgpack::map_size_overflow("map size overflow");
    }
    obj->via.map.ptr = static_cast<msgpack::object_kv *>(m_zone.allocate_align(size, alignof(msgpack::object_kv)));
  }
  m_stack.push_back(reinterpret_cast<msgpack::object *>(obj->via.map.ptr));
  return true;
}

void object_visitor::parse_error(size_t /*parsed_offset*/, size_t /*error_offset*/) const {
  throw msgpack::parse_error("parse error");
}
void object_visitor::insufficient_bytes(size_t /*parsed_offset*/, size_t /*error_offset*/) const {
  throw msgpack::insufficient_bytes("insufficient bytes");
}

} // namespace msgpack
