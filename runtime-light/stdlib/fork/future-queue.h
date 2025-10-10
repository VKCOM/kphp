// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>
#include <type_traits>
#include <utility>

#include "runtime-light/stdlib/fork/fork-state.h"
#include "runtime-light/stdlib/fork/wait-queue-state.h"

template<typename T>
struct future_queue final : public refcountable_php_classes<C$ComponentQuery> {
  using result_type = T;
  int64_t m_future_id;

  future_queue() noexcept
      : m_future_id(kphp::forks::INVALID_ID) {}

  future_queue(int64_t id) noexcept
      : m_future_id(id) {}

  template<typename U>
  future_queue(const future_queue<U>& other) noexcept
      : m_future_id(other.m_future_id) {}

  template<typename U>
  future_queue(future_queue<U>&& other) noexcept
      : m_future_id(std::exchange(other.m_future_id, kphp::forks::INVALID_ID)) {}

  int64_t future_id() noexcept {
    return m_future_id;
  }

  ~future_queue() {
    if (m_future_id != kphp::forks::INVALID_ID) {
      WaitQueueInstanceState::get().erase_queue(m_future_id);
    }
  }
};

template<class T>
struct is_future_queue : std::false_type {};

template<class T>
struct is_future_queue<future_queue<T>> : std::true_type {};
