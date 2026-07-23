//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2026 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cassert>
#include <memory>
#include <type_traits>
#include <utility>

namespace kphp::stl::intrusive_list {

namespace details {

class hook_base {
  hook_base* m_prev{this};
  hook_base* m_next{this};

  void move_hook(hook_base&& other) noexcept;

public:
  hook_base() noexcept = default;

  hook_base(const hook_base& /*unused*/) noexcept;

  hook_base(hook_base&& other) noexcept;

  hook_base& operator=(const hook_base& /*unused*/) noexcept;

  hook_base& operator=(hook_base&& other) noexcept;

  ~hook_base();

  bool is_linked() const noexcept;

  void unlink() noexcept;
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
