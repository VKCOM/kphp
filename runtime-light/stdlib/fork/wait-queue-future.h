// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>

#include "runtime-light/stdlib/fork/fork-state.h"

namespace kphp::forks {

template<typename T>
struct wait_queue_future final {
  int64_t m_future_id;

  wait_queue_future() noexcept
      : m_future_id(kphp::forks::INVALID_ID) {}

  wait_queue_future(int64_t id) noexcept // NOLINT
      : m_future_id(id) {}

  wait_queue_future(const wait_queue_future& other) noexcept // NOLINT
      : m_future_id(other.m_future_id) {}

  wait_queue_future(wait_queue_future&& other) noexcept // NOLINT
      : m_future_id(std::exchange(other.m_future_id, kphp::forks::INVALID_ID)) {}

  template<typename U>
  requires(!std::is_same_v<T, Unknown> && std::is_same_v<U, Unknown>)
  wait_queue_future(const wait_queue_future<U>& other) noexcept // NOLINT
      : m_future_id(other.m_future_id) {}

  template<typename U>
  requires(!std::is_same_v<T, Unknown> && std::is_same_v<U, Unknown>)
  wait_queue_future(wait_queue_future<U>&& other) noexcept // NOLINT
      : m_future_id(std::exchange(other.m_future_id, kphp::forks::INVALID_ID)) {}

  wait_queue_future& operator=(const wait_queue_future& other) noexcept {
    m_future_id = other.m_future_id;
    return *this;
  }

  wait_queue_future& operator=(wait_queue_future&& other) noexcept {
    m_future_id = std::exchange(other.m_future_id, kphp::forks::INVALID_ID);
    return *this;
  }
};

template<class T>
struct is_wait_queue_future : std::false_type {};

template<class T>
struct is_wait_queue_future<wait_queue_future<T>> : std::true_type {};

template<typename T>
constexpr bool is_wait_queue_future_v = is_wait_queue_future<T>::value;

} // namespace kphp::forks

// To make it visible for codegen
template<typename T>
using future_queue = kphp::forks::wait_queue_future<T>;
