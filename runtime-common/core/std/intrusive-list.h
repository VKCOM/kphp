//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2026 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace kphp::stl::intrusive_list {

namespace details {

class hook_base {
  hook_base* m_prev{this};
  hook_base* m_next{this};

  auto move_hook(hook_base&& other) noexcept -> void;

public:
  hook_base() noexcept = default;

  hook_base(const hook_base& /*unused*/) noexcept;

  hook_base(hook_base&& other) noexcept;

  auto operator=(const hook_base& /*unused*/) noexcept -> hook_base&;

  auto operator=(hook_base&& other) noexcept -> hook_base&;

  ~hook_base();

  auto is_linked() const noexcept -> bool;

  auto unlink() noexcept -> void;
};

} // namespace details

struct default_tag {};

template<typename Tag = default_tag>
class hook : private details::hook_base {};

template<typename T, typename Tag = default_tag>
class intrusive_list final {
  static_assert(std::is_base_of_v<hook<Tag>, T>, "T must be derived from hook<Tag>");

  details::hook_base m_sentinel;

public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using iterator = void;
  using const_iterator = void;
  using reverse_iterator = void;
  using const_reverse_iterator = void;

  intrusive_list();
};

} // namespace kphp::stl::intrusive_list
