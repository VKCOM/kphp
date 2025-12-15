// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <utility>

#include "common/php-functions.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::core {

template<typename T>
void set_reference_counter_recursive(T& /*obj*/, ExtraRefCnt /*rc*/) noexcept {}

template<typename T>
void set_reference_counter_recursive(Optional<T>& obj, ExtraRefCnt rc) noexcept {
  if (obj.has_value()) {
    set_reference_counter_recursive(obj.val(), rc);
  }
}

template<typename T>
void set_reference_counter_recursive(class_instance<T>& obj, ExtraRefCnt /*rc*/) noexcept {
  if (!obj.is_null()) {
    // TODO we need some visitor to visit all the class members
    kphp::log::assertion(false);
  }
}

template<typename T>
void set_reference_counter_recursive(array<T>& obj, ExtraRefCnt rc) noexcept {
  obj.set_reference_counter_to(rc);
  for (const auto& it : std::as_const(obj)) {
    auto key{it.get_key()};
    set_reference_counter_recursive(key, rc);
    auto value{it.get_value()};
    set_reference_counter_recursive(value, rc);
  }
}

template<>
inline void set_reference_counter_recursive<string>(string& obj, ExtraRefCnt rc) noexcept {
  return obj.set_reference_counter_to(rc);
}

template<>
inline void set_reference_counter_recursive<mixed>(mixed& obj, ExtraRefCnt rc) noexcept {
  switch (obj.get_type()) {
  case mixed::type::NUL:
  case mixed::type::BOOLEAN:
  case mixed::type::INTEGER:
  case mixed::type::FLOAT:
    return;
  case mixed::type::STRING:
  case mixed::type::OBJECT:
    return obj.set_reference_counter_to(rc);
  case mixed::type::ARRAY:
    return set_reference_counter_recursive(obj.as_array(), rc);
  }
}

// ================================================================================================

template<typename T>
bool is_reference_counter_recursive(const T& /*obj*/, ExtraRefCnt /*rc*/) noexcept {
  return true;
}

template<typename T>
bool is_reference_counter_recursive(const Optional<T>& obj, ExtraRefCnt rc) noexcept {
  return obj.has_value() ? is_reference_counter_recursive(obj.val(), rc) : true;
}

template<typename T>
bool is_reference_counter_recursive(const class_instance<T>& obj, ExtraRefCnt /*rc*/) noexcept {
  if (!obj.is_null()) {
    // TODO we need some visitor to visit all the class members
    kphp::log::assertion(false);
  }
}

template<typename T>
bool is_reference_counter_recursive(const array<T>& obj, ExtraRefCnt rc) noexcept {
  if (!obj.is_reference_counter(rc)) {
    return false;
  }

  for (const auto& it : obj) { // NOLINT
    if (!is_reference_counter_recursive(it.get_key(), rc) || !is_reference_counter_recursive(it.get_value(), rc)) {
      return false;
    }
  }
  return true;
}

template<>
inline bool is_reference_counter_recursive<string>(const string& obj, ExtraRefCnt rc) noexcept {
  return obj.is_reference_counter(rc);
}

template<>
inline bool is_reference_counter_recursive<mixed>(const mixed& obj, ExtraRefCnt rc) noexcept {
  switch (obj.get_type()) {
  case mixed::type::NUL:
  case mixed::type::BOOLEAN:
  case mixed::type::INTEGER:
  case mixed::type::FLOAT:
    return true;
  case mixed::type::STRING:
  case mixed::type::OBJECT:
    return obj.is_reference_counter(rc);
  case mixed::type::ARRAY:
    return is_reference_counter_recursive(obj.as_array(), rc);
  }
}

} // namespace kphp::core
