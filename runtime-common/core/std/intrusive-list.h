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

template<typename T, typename... Tags>
class list_node;

namespace details {

class list_node_base {
  list_node_base* m_prev{this};
  list_node_base* m_next{this};

  constexpr auto insert_instead(list_node_base&& other) noexcept -> void;

  template<typename, typename>
  friend class list_iterator;

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

template<typename Tag>
struct tag_holder : public list_node_base {};

template<typename... Tags>
struct tag_holder_variadic : public tag_holder<Tags>... {};

template<typename Node, typename Tag>
constexpr auto as_value(list_node_base& node) noexcept -> Node::element_type& {
  using tag_holder_variadic_t = vk::apply_tuple_t<tag_holder_variadic, typename Node::tags>;
  return (static_cast<Node&>(static_cast<tag_holder_variadic_t&>(static_cast<tag_holder<Tag>&>(node)))).get();
}

template<typename Node, typename Tag>
constexpr auto as_value(const list_node_base& node) noexcept -> const Node::element_type& {
  using tag_holder_variadic_t = vk::apply_tuple_t<tag_holder_variadic, typename Node::tags>;
  return (static_cast<const Node&>(static_cast<const tag_holder_variadic_t&>(static_cast<const tag_holder<Tag>&>(node)))).get();
}

} // namespace details

struct default_tag {};

template<typename T, typename... Tags>
class list_node final : private std::conditional_t<sizeof...(Tags) == 0, details::tag_holder_variadic<default_tag>, details::tag_holder_variadic<Tags...>> {
  T m_value;

public:
  using element_type = T;
  using tags = std::tuple<Tags...>;

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

template<typename T, typename... Tags>
constexpr auto make_list_node(T value) noexcept -> list_node<T, Tags...> {
  return list_node<T, Tags...>{std::move(value)};
}

template<typename Node, typename Tag = default_tag>
class list_iterator {
  using node_type = std::conditional_t<std::is_const_v<Node>, details::list_node_base, const details::list_node_base>;

  node_type* m_curr;

public:
  using difference_type = std::ptrdiff_t;
  using value_type = Node::element_type;
  using pointer = value_type*;
  using reference = value_type&;
  using iterator_category = std::bidirectional_iterator_tag;

  constexpr explicit list_iterator(node_type& node) noexcept
      : m_curr{std::addressof(node)} {}

  constexpr list_iterator(const list_iterator& other) noexcept = default;

  constexpr list_iterator(list_iterator&& other) noexcept = default;

  constexpr auto operator=(const list_iterator& other) noexcept -> list_iterator& = default;

  constexpr auto operator=(list_iterator&& other) noexcept -> list_iterator& = default;

  ~list_iterator() = default;

  constexpr auto operator++() noexcept -> list_iterator& {
    m_curr = m_curr->m_next;
    return *this;
  }

  constexpr auto operator++(int) noexcept -> list_iterator {
    list_iterator res = *this;
    ++*this;
    return res;
  }

  constexpr auto operator--() noexcept -> list_iterator& {
    m_curr = m_curr->m_prev;
    return *this;
  }

  constexpr auto operator--(int) noexcept -> list_iterator {
    list_iterator res = *this;
    --*this;
    return res;
  }

  constexpr auto operator*() const noexcept -> reference {
    return details::as_value<Node, Tag>(*m_curr);
  }

  constexpr auto operator==(const list_iterator& other) const noexcept -> bool {
    return m_curr == other.m_curr;
  }

  constexpr auto operator!=(const list_iterator& other) const noexcept -> bool {
    return !(*this == other);
  }
};

template<typename Node, typename Tag = default_tag>
class list final {
  details::list_node_base m_sentinel;

public:
  using value_type = Node::element_type;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using iterator = list_iterator<Node>;
  using const_iterator = list_iterator<const Node>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  constexpr list() noexcept = default;

  template<typename It>
  constexpr list(It first, It last) noexcept;

  list(const list& other) = delete;

  constexpr list(list&& other) noexcept = default;

  auto operator=(const list& other) -> list& = delete;

  constexpr auto operator=(list&& other) noexcept -> list& = default;

  ~list() = default;

  constexpr auto front() noexcept -> reference;

  constexpr auto front() const noexcept -> const_reference;

  constexpr auto back() noexcept -> reference;

  constexpr auto back() const noexcept -> const_reference;

  constexpr auto begin() noexcept -> iterator;

  constexpr auto begin() const noexcept -> const_iterator;

  constexpr auto cbegin() const noexcept -> const_iterator;

  constexpr auto end() noexcept -> iterator;

  constexpr auto end() const noexcept -> const_iterator;

  constexpr auto cend() const noexcept -> const_iterator;

  constexpr auto rbegin() noexcept -> reverse_iterator;

  constexpr auto rbegin() const noexcept -> const_reverse_iterator;

  constexpr auto crbegin() const noexcept -> const_reverse_iterator;

  constexpr auto rend() noexcept -> reverse_iterator;

  constexpr auto rend() const noexcept -> const_reverse_iterator;

  constexpr auto crend() const noexcept -> const_reverse_iterator;

  constexpr auto empty() const noexcept -> bool;

  constexpr auto size() const noexcept -> size_type;

  constexpr auto clear() noexcept -> void;

  constexpr auto insert(const_iterator pos, const Node& node) noexcept -> iterator;

  template<typename It>
  constexpr auto insert(const_iterator pos, It first, It last) noexcept -> iterator;

  constexpr auto erase(const_iterator pos) noexcept -> iterator;

  constexpr auto erase(const_iterator first, const_iterator last) noexcept -> iterator;

  constexpr auto push_back(const Node& node) noexcept -> void;

  constexpr auto push_front(const Node& node) noexcept -> void;

  constexpr auto pop_back() noexcept -> void;

  constexpr auto pop_front() noexcept -> void;

  constexpr auto swap(list& other) noexcept -> void;

  constexpr auto splice(const_iterator pos, list& other) noexcept -> void;

  constexpr auto splice(const_iterator pos, list&& other) noexcept -> void;

  constexpr auto splice(const_iterator pos, list& other, const_iterator it) noexcept -> void;

  constexpr auto splice(const_iterator pos, list&& other, const_iterator it) noexcept -> void;

  constexpr auto splice(const_iterator pos, list& other, const_iterator first, const_iterator last) noexcept -> void;

  constexpr auto splice(const_iterator pos, list&& other, const_iterator first, const_iterator last) noexcept -> void;
};

template<typename Node, typename Tag>
constexpr auto swap(list<Node, Tag>& lhs, list<Node, Tag>& rhs) noexcept -> void;

} // namespace kphp::stl::intrusive
