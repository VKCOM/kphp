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

namespace vk::intrusive {

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
  friend class vk::intrusive::list;

  template<typename, typename>
  friend class vk::intrusive::list_iterator;

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
  T m_value{};

  template<typename, typename>
  friend class vk::intrusive::list;

  template<typename, typename>
  friend class vk::intrusive::list_iterator;

public:
  using value_type = T;
  using tags = std::conditional_t<sizeof...(Tags) == 0, std::tuple<default_tag>, std::tuple<Tags...>>;

  list_node() noexcept = default;

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

namespace details {

template<typename>
struct is_list_node : std::false_type {};

template<typename T, typename... Tags>
struct is_list_node<list_node<T, Tags...>> : std::true_type {};

template<typename T>
inline constexpr bool is_list_node_v = is_list_node<std::decay_t<T>>::value;

template<typename Tag, typename... Tags>
struct is_tag_of : std::false_type {};

template<typename Tag, typename... Tags>
struct is_tag_of<Tag, std::tuple<Tags...>> : std::bool_constant<(std::is_same_v<Tag, Tags> || ...)> {};

template<typename Tag, typename Node>
inline constexpr bool is_tag_of_v = is_tag_of<Tag, typename std::decay_t<Node>::tags>::value;

} // namespace details

template<typename Node, typename Tag = default_tag>
class list_iterator {
public:
  using difference_type = std::ptrdiff_t;
  using value_type = typename Node::value_type;
  using pointer = std::conditional_t<std::is_const_v<Node>, const value_type*, value_type*>;
  using reference = std::conditional_t<std::is_const_v<Node>, const value_type&, value_type&>;
  using iterator_category = std::bidirectional_iterator_tag;

private:
  static_assert(details::is_list_node_v<Node>, "Node must be a specialization of list_node");
  static_assert(details::is_tag_of_v<Tag, Node>, "Tag is not one of Node's tags");

  details::list_node_base* m_curr{nullptr};

  explicit list_iterator(const details::list_node_base* node) noexcept
      : m_curr{const_cast<details::list_node_base*>(node)} {}

  static auto value_from_list_node_base(details::list_node_base* node) noexcept -> reference {
    using tagged_hooks_t = vk::apply_tuple_t<details::tagged_hooks, typename Node::tags>;
    /*
     * save cast to local variable to avoid compilation error in g++-11:
     * error: ‘this’ pointer is null [-Werror=nonnull]
     * 150 |     return (static_cast<Node*>(static_cast<tagged_hooks_t*>(static_cast<details::tagged_hook<Tag>*>(node))))->value();
     */
    auto* list_node = static_cast<Node*>(static_cast<tagged_hooks_t*>(static_cast<details::tagged_hook<Tag>*>(node)));
    return list_node->value();
  }

  template<typename, typename>
  friend class vk::intrusive::list_iterator;

  template<typename, typename>
  friend class vk::intrusive::list;

public:
  list_iterator() noexcept = default;

  list_iterator(const list_iterator& other) noexcept = default;

  template<typename NodeU, typename = std::enable_if_t<std::is_const_v<Node> && !std::is_const_v<NodeU>>>
  list_iterator(const list_iterator<NodeU, Tag>& other) noexcept // NOLINT (hicpp-explicit-conversions)
      : m_curr{other.m_curr} {}

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
    return value_from_list_node_base(m_curr);
  }

  template<typename NodeU, typename = std::enable_if_t<std::is_same_v<std::remove_const_t<Node>, std::remove_const_t<NodeU>>>>
  auto operator==(const list_iterator<NodeU, Tag>& other) const noexcept -> bool {
    return m_curr == other.m_curr;
  }

  template<typename NodeU, typename = std::enable_if_t<std::is_same_v<std::remove_const_t<Node>, std::remove_const_t<NodeU>>>>
  auto operator!=(const list_iterator<NodeU, Tag>& other) const noexcept -> bool {
    return !(*this == other);
  }
};

template<typename Node, typename Tag = default_tag>
class list {
public:
  using value_type = typename Node::value_type;
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

private:
  static_assert(details::is_list_node_v<Node>, "Node must be a specialization of list_node");
  static_assert(details::is_tag_of_v<Tag, Node>, "Tag is not one of Node's tags");

  details::list_node_base m_sentinel;

  static auto list_node_base_from_list_node(Node& node) noexcept -> details::list_node_base* {
    using tagged_hooks_t = vk::apply_tuple_t<details::tagged_hooks, typename Node::tags>;
    return static_cast<details::list_node_base*>(static_cast<details::tagged_hook<Tag>*>(static_cast<tagged_hooks_t*>(std::addressof(node))));
  }

  static auto list_node_base_from_list_node(const Node& node) noexcept -> const details::list_node_base* {
    using tagged_hooks_t = vk::apply_tuple_t<details::tagged_hooks, typename Node::tags>;
    return static_cast<const details::list_node_base*>(static_cast<const details::tagged_hook<Tag>*>(static_cast<const tagged_hooks_t*>(std::addressof(node))));
  }

  auto splice_impl(const_iterator pos, list& other, const_iterator first, const_iterator last) noexcept -> void {
    if (first == last || (this == std::addressof(other) && (first == pos || last == pos))) {
      return;
    }

    auto* pos_node = pos.m_curr;
    auto* first_node = first.m_curr;
    auto* last_node = last.m_curr->m_prev;

    first_node->m_prev->m_next = last_node->m_next;
    last_node->m_next->m_prev = first_node->m_prev;

    first_node->m_prev = pos_node->m_prev;
    last_node->m_next = pos_node;

    pos_node->m_prev->m_next = first_node;
    pos_node->m_prev = last_node;
  }

public:
  list() noexcept = default;

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
    return *std::prev(end());
  }

  auto back() const noexcept -> const_reference {
    return *std::prev(end());
  }

  auto begin() noexcept -> iterator {
    return iterator{m_sentinel.m_next};
  }

  auto begin() const noexcept -> const_iterator {
    return const_iterator{m_sentinel.m_next};
  }

  auto cbegin() const noexcept -> const_iterator {
    return begin();
  }

  auto end() noexcept -> iterator {
    return iterator{std::addressof(m_sentinel)};
  }

  auto end() const noexcept -> const_iterator {
    return const_iterator{std::addressof(m_sentinel)};
  }

  auto cend() const noexcept -> const_iterator {
    return end();
  }

  auto rbegin() noexcept -> reverse_iterator {
    return reverse_iterator{end()};
  }

  auto rbegin() const noexcept -> const_reverse_iterator {
    return const_reverse_iterator{end()};
  }

  auto crbegin() const noexcept -> const_reverse_iterator {
    return rbegin();
  }

  auto rend() noexcept -> reverse_iterator {
    return reverse_iterator{begin()};
  }

  auto rend() const noexcept -> const_reverse_iterator {
    return const_reverse_iterator{begin()};
  }

  auto crend() const noexcept -> const_reverse_iterator {
    return rend();
  }

  auto empty() const noexcept -> bool {
    return !m_sentinel.is_linked();
  }

  // complexity O(n)
  auto size() const noexcept -> size_type {
    return std::distance(begin(), end());
  }

  auto clear() noexcept -> void {
    m_sentinel.unlink();
  }

  // if the node is linked in other list with the same tag, it will be unlinked from other list before insertion
  auto insert(const_iterator pos, Node& node) noexcept -> iterator {
    auto* next_node = pos.m_curr;
    auto* new_node = list_node_base_from_list_node(node);
    if (new_node == next_node) {
      return iterator{next_node};
    }

    new_node->unlink();

    new_node->m_prev = next_node->m_prev;
    new_node->m_next = next_node;

    new_node->m_prev->m_next = new_node;
    new_node->m_next->m_prev = new_node;

    return iterator{new_node};
  }

  auto erase(const_iterator pos) noexcept -> iterator {
    auto* remove_node = pos.m_curr;
    auto* next_node = remove_node->m_next;

    remove_node->unlink();

    return iterator{next_node};
  }

  auto erase(const_iterator first, const_iterator last) noexcept -> iterator {
    while (first != last) {
      first = erase(first);
    }

    return iterator{last.m_curr};
  }

  auto iterator_to(Node& node) noexcept -> iterator {
    return iterator{list_node_base_from_list_node(node)};
  }

  auto iterator_to(const Node& node) const noexcept -> const_iterator {
    return const_iterator{list_node_base_from_list_node(node)};
  }

  auto push_back(Node& node) noexcept -> void {
    insert(end(), node);
  }

  auto push_front(Node& node) noexcept -> void {
    insert(begin(), node);
  }

  auto pop_back() noexcept -> void {
    erase(std::prev(end()));
  }

  auto pop_front() noexcept -> void {
    erase(begin());
  }

  auto swap(list& other) noexcept -> void {
    if (this == std::addressof(other)) {
      return;
    }

    details::list_node_base tmp{std::move(m_sentinel)};
    m_sentinel = std::move(other.m_sentinel);
    other.m_sentinel = std::move(tmp);
  }

  auto splice(const_iterator pos, list& other) noexcept -> void {
    splice(pos, other, other.begin(), other.end());
  }

  auto splice(const_iterator pos, list&& other) noexcept -> void {
    splice(pos, other, other.begin(), other.end());
  }

  auto splice(const_iterator pos, list& other, const_iterator it) noexcept -> void {
    splice(pos, other, it, std::next(it));
  }

  auto splice(const_iterator pos, list&& other, const_iterator it) noexcept -> void {
    splice(pos, other, it, std::next(it));
  }

  auto splice(const_iterator pos, list& other, const_iterator first, const_iterator last) noexcept -> void {
    splice_impl(pos, other, first, last);
  }

  auto splice(const_iterator pos, list&& other, const_iterator first, const_iterator last) noexcept -> void {
    splice_impl(pos, other, first, last);
  }
};

template<typename Node, typename Tag>
auto swap(list<Node, Tag>& lhs, list<Node, Tag>& rhs) noexcept -> void {
  lhs.swap(rhs);
}

} // namespace vk::intrusive
