//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2026 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace kphp::stl::intrusive {

namespace details {

class list_node_base {
  list_node_base* m_prev{this};
  list_node_base* m_next{this};

  constexpr auto insert_instead(list_node_base&& other) noexcept -> void;

public:
  constexpr list_node_base() noexcept = default;

  list_node_base(const list_node_base& /*unused*/) noexcept = delete;

  constexpr list_node_base(list_node_base&& other) noexcept;

  auto operator=(const list_node_base& /*unused*/) noexcept -> list_node_base& = delete;

  constexpr auto operator=(list_node_base&& other) noexcept -> list_node_base&;

  ~list_node_base();

  constexpr auto is_linked() const noexcept -> bool;

  constexpr auto unlink() noexcept -> void;
};

template<typename T>
class list_node final : public list_node_base {
  T m_value;

public:
  constexpr explicit list_node(T value) noexcept
      : m_value{std::move(value)} {}

  list_node(const list_node& other) = delete;

  list_node(list_node&& other) = delete;

  auto operator=(const list_node& other) -> list_node& = delete;

  auto operator=(list_node&& other) -> list_node& = delete;

  constexpr ~list_node() = default;

  constexpr auto get() noexcept -> T& {
    return m_value;
  }

  constexpr auto get() const noexcept -> const T& {
    return m_value;
  }
};

template<typename T>
class list_node<T&> final : public list_node_base {
  T& m_value;

public:
  constexpr explicit list_node(T& value) noexcept
      : m_value{value} {}

  list_node(const list_node& other) = delete;

  list_node(list_node&& other) = delete;

  auto operator=(const list_node& other) -> list_node& = delete;

  auto operator=(list_node&& other) -> list_node& = delete;

  constexpr ~list_node() = default;

  constexpr auto get() noexcept -> T& {
    return m_value;
  }

  constexpr auto get() const noexcept -> const T& {
    return m_value;
  }
};

} // namespace details

// Node for object that is stored in single intrusive list
template<typename T>
using owning_list_node = details::list_node<std::remove_pointer_t<std::remove_reference_t<T>>>;

// Node for object that is stored in multiple intrusive lists
template<typename T>
using non_owning_list_node = details::list_node<std::remove_pointer_t<std::remove_reference_t<T>>&>;

template<typename T>
constexpr auto make_owning_list_node(T value) noexcept -> owning_list_node<T> {
  return owning_list_node<T>{std::move(value)};
}

template<typename T>
constexpr auto make_non_owning_list_node(T& value) noexcept -> non_owning_list_node<T> {
  return non_owning_list_node<T>{value};
}

template<typename T>
class list_iterator;

template<typename T>
class list final {
  details::list_node_base m_sentinel;

public:
  using value_type = T;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using iterator = list_iterator<T>;
  using const_iterator = list_iterator<const T>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  constexpr list() noexcept = default;

  template<typename It>
  constexpr list(It first, It last);

  list(const list& other) = delete;

  constexpr list(list&& other) noexcept = default;

  auto operator=(const list& other) -> list& = delete;

  constexpr auto operator=(list&& other) noexcept -> list& = default;

  ~list() = default;

  constexpr auto front() -> reference;

  constexpr auto front() const -> const_reference;

  constexpr auto back() -> reference;

  constexpr auto back() const -> const_reference;

  constexpr auto begin() -> iterator;

  constexpr auto begin() const -> const_iterator;

  constexpr auto cbegin() const -> const_iterator;

  constexpr auto end() -> iterator;

  constexpr auto end() const -> const_iterator;

  constexpr auto cend() const -> const_iterator;

  constexpr auto rbegin() -> reverse_iterator;

  constexpr auto rbegin() const -> const_reverse_iterator;

  constexpr auto crbegin() const -> const_reverse_iterator;

  constexpr auto rend() -> reverse_iterator;

  constexpr auto rend() const -> const_reverse_iterator;

  constexpr auto crend() const -> const_reverse_iterator;

  constexpr auto empty() const -> bool;

  constexpr auto size() const -> size_type;

  constexpr auto clear() -> void;

  constexpr auto insert(const_iterator pos, const_reference value) -> iterator;

  template<typename It>
  constexpr auto insert(const_iterator pos, It first, It last) -> iterator;

  constexpr auto erase(const_iterator pos) -> iterator;

  constexpr auto erase(const_iterator first, const_iterator last) -> iterator;

  constexpr auto push_back(const_reference value) -> void;

  constexpr auto push_front(const_reference value) -> void;

  constexpr auto pop_back() -> void;

  constexpr auto pop_front() -> void;

  constexpr auto swap(list& other) -> void;

  constexpr auto merge(list& other) -> void;
};

} // namespace kphp::stl::intrusive
