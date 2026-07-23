//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2026 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>
#include <type_traits>
#include <utility>

namespace kphp::stl::intrusive_list {

namespace details {

class hook_base {
  hook_base* m_prev{this};
  hook_base* m_next{this};

  // correct usage supposes that this->is_linked() == false
  void move_hook(hook_base&& other) noexcept {
    if (other.is_linked()) {
        m_prev = std::exchange(other.m_prev, std::addressof(other));
        m_next = std::exchange(other.m_next, std::addressof(other));

        m_prev->m_next = this;
        m_next->m_prev = this;
    }
  }

public:
  hook_base() noexcept = default;

  hook_base(const hook_base& /*unused*/) noexcept : hook_base() {}

  hook_base(hook_base&& other) noexcept {
    move_hook(std::move(other));
  }

  hook_base& operator=(const hook_base& /*unused*/) noexcept {
    return *this;
  }

  hook_base& operator=(hook_base&& other) noexcept {
    if (this == std::addressof(other)) {
        return *this;
    }

    unlink();
    move_hook(std::move(other));

    return *this;
  }

  ~hook_base() {
    unlink();
  }

  bool is_linked() const noexcept {
    return m_prev != this;
  }

  void unlink() noexcept {
    m_prev->m_next = m_next;
    m_next->m_prev = m_prev;

    m_prev = this;
    m_next = this;
  }
};

} // namespace details

struct default_tag {};

template<typename Tag = default_tag>
class hook : private details::hook_base {};

template<typename T, typename Tag = default_tag>
class intrusive_list final {
  static_assert(std::is_base_of_v<hook<Tag>, T>, "T must be derived from hook<Tag>");
};

} // namespace kphp::stl::intrusive_list
