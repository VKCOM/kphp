//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2026 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include "common/type_traits/apply_tuple.h"

namespace kphp::stl::intrusive {

template<typename, typename>
class list;

template<typename, typename>
class list_iterator;

namespace details {

class list_node_base {
  list_node_base* m_prev{this};
  list_node_base* m_next{this};

  auto take_place_of(list_node_base&& other) noexcept -> void;

  template<typename, typename>
  friend class kphp::stl::intrusive::list;

  template<typename, typename>
  friend class kphp::stl::intrusive::list_iterator;

public:
  list_node_base() noexcept = default;

  list_node_base(const list_node_base& /*unused*/) noexcept = delete;

  list_node_base(list_node_base&& other) noexcept;

  auto operator=(const list_node_base& /*unused*/) noexcept -> list_node_base& = delete;

  auto operator=(list_node_base&& other) noexcept -> list_node_base&;

  ~list_node_base();

  auto is_linked() const noexcept -> bool;

  auto unlink() noexcept -> void;
};

template<typename Tag>
struct tagged_hook : public list_node_base {};

template<typename... Tags>
struct tagged_hooks : public tagged_hook<Tags>... {};

} // namespace details

struct default_tag {};

template<typename T, typename... Tags>
class list_node final : private std::conditional_t<sizeof...(Tags) == 0, details::tagged_hooks<default_tag>, details::tagged_hooks<Tags...>> {
  T m_value;

public:
  using value_type = T;
  using tags = std::conditional_t<sizeof...(Tags) == 0, std::tuple<default_tag>, std::tuple<Tags...>>;

  explicit list_node(T value) noexcept
      : m_value{std::move(value)} {}

  list_node(const list_node& other) = delete;

  list_node(list_node&& other) noexcept = default;

  auto operator=(const list_node& other) -> list_node& = delete;

  auto operator=(list_node&& other) noexcept -> list_node& = default;

  ~list_node() = default;

  auto value() noexcept -> T& {
    return m_value;
  }

  auto value() const noexcept -> const T& {
    return m_value;
  }
};

template<typename T, typename... Tags>
auto make_list_node(T value) noexcept -> list_node<T, Tags...> {
  return list_node<T, Tags...>{std::move(value)};
}

template<typename Node, typename Tag = default_tag>
class list_iterator final {
  using list_node_base_type = std::conditional_t<std::is_const_v<Node>, const details::list_node_base, details::list_node_base>;

  list_node_base_type* m_curr;

  static auto value_from_list_node_base(details::list_node_base& node) noexcept -> Node::element_type& {
    using tag_holder_variadic_t = vk::apply_tuple_t<details::tagged_hooks, typename Node::tags>;
    return (static_cast<Node&>(static_cast<tag_holder_variadic_t&>(static_cast<details::tagged_hook<Tag>&>(node)))).value();
  }

  static auto value_from_list_node_base(const details::list_node_base& node) noexcept -> const Node::element_type& {
    using tag_holder_variadic_t = vk::apply_tuple_t<details::tagged_hooks, typename Node::tags>;
    return (static_cast<const Node&>(static_cast<const tag_holder_variadic_t&>(static_cast<const details::tagged_hook<Tag>&>(node)))).value();
  }

public:
  using difference_type = std::ptrdiff_t;
  using value_type = Node::value_type;
  using pointer = value_type*;
  using reference = value_type&;
  using iterator_category = std::bidirectional_iterator_tag;

  explicit list_iterator(list_node_base_type& node) noexcept
      : m_curr{std::addressof(node)} {}

  list_iterator(const list_iterator& other) noexcept = default;

  list_iterator(list_iterator&& other) noexcept = default;

  auto operator=(const list_iterator& other) noexcept -> list_iterator& = default;

  auto operator=(list_iterator&& other) noexcept -> list_iterator& = default;

  ~list_iterator() = default;

  auto operator++() noexcept -> list_iterator& {
    m_curr = m_curr->m_next;
    return *this;
  }

  auto operator++(int) noexcept -> list_iterator {
    list_iterator res = *this;
    ++*this;
    return res;
  }

  auto operator--() noexcept -> list_iterator& {
    m_curr = m_curr->m_prev;
    return *this;
  }

  auto operator--(int) noexcept -> list_iterator {
    list_iterator res = *this;
    --*this;
    return res;
  }

  auto operator*() const noexcept -> reference {
    return value_from_list_node_base(*m_curr);
  }

  auto operator==(const list_iterator& other) const noexcept -> bool {
    return m_curr == other.m_curr;
  }

  auto operator!=(const list_iterator& other) const noexcept -> bool {
    return !(*this == other);
  }
};

template<typename Node, typename Tag = default_tag>
class list final {
  details::list_node_base m_sentinel;

public:
  using value_type = Node::value_type;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using iterator = list_iterator<Node, Tag>;
  using const_iterator = list_iterator<const Node, Tag>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  list() noexcept = default;

  template<typename It>
  list(It first, It last) noexcept;

  list(const list& other) = delete;

  list(list&& other) noexcept = default;

  auto operator=(const list& other) -> list& = delete;

  auto operator=(list&& other) noexcept -> list& = default;

  ~list() = default;

  auto front() noexcept -> reference {
    return *begin();
  }

  auto front() const noexcept -> const_reference {
    return *begin();
  }

  auto back() noexcept -> reference {
    return *(--end());
  }

  auto back() const noexcept -> const_reference {
    return *(--end());
  }

  auto begin() noexcept -> iterator {
    return iterator{*m_sentinel.m_next};
  }

  auto begin() const noexcept -> const_iterator {
    return const_iterator{*m_sentinel.m_next};
  }

  auto cbegin() const noexcept -> const_iterator {
    return begin();
  }

  auto end() noexcept -> iterator {
    return iterator{m_sentinel};
  }

  auto end() const noexcept -> const_iterator {
    return const_iterator{m_sentinel};
  }

  auto cend() const noexcept -> const_iterator {
    return end();
  }

  auto rbegin() noexcept -> reverse_iterator {
    return reverse_iterator{--end()};
  }

  auto rbegin() const noexcept -> const_reverse_iterator {
    return const_reverse_iterator{--end()};
  }

  auto crbegin() const noexcept -> const_reverse_iterator {
    return rbegin();
  }

  auto rend() noexcept -> reverse_iterator {
    return reverse_iterator{--begin()};
  }

  auto rend() const noexcept -> const_reverse_iterator {
    return const_reverse_iterator{--begin()};
  }

  auto crend() const noexcept -> const_reverse_iterator {
    return rend();
  }

  auto empty() const noexcept -> bool {
    return !m_sentinel.is_linked();
  }

  auto size() const noexcept -> size_type {
    return std::distance(begin(), end());
  }

  auto clear() noexcept -> void {
    m_sentinel.unlink();
  }

  auto insert(const_iterator pos, const Node& node) noexcept -> iterator;

  template<typename It>
  auto insert(const_iterator pos, It first, It last) noexcept -> iterator;

  auto erase(const_iterator pos) noexcept -> iterator;

  auto erase(const_iterator first, const_iterator last) noexcept -> iterator;

  auto push_back(const Node& node) noexcept -> void;

  auto push_front(const Node& node) noexcept -> void;

  auto pop_back() noexcept -> void;

  auto pop_front() noexcept -> void;

  auto swap(list& other) noexcept -> void;

  auto splice(const_iterator pos, list& other) noexcept -> void;

  auto splice(const_iterator pos, list&& other) noexcept -> void;

  auto splice(const_iterator pos, list& other, const_iterator it) noexcept -> void;

  auto splice(const_iterator pos, list&& other, const_iterator it) noexcept -> void;

  auto splice(const_iterator pos, list& other, const_iterator first, const_iterator last) noexcept -> void;

  auto splice(const_iterator pos, list&& other, const_iterator first, const_iterator last) noexcept -> void;
};

template<typename Node, typename Tag>
auto swap(list<Node, Tag>& lhs, list<Node, Tag>& rhs) noexcept -> void;

} // namespace kphp::stl::intrusive
