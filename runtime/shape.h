#pragma once

#include <cstdint>
#include <cstdlib>
#include <type_traits>
#include <utility>

template<size_t tag, typename T>
class shape_node {
public:
  T value{};
  shape_node() = default;
  template<typename T1, typename = std::is_constructible<T, T1 &&>>
  explicit shape_node(T1 value) :
    value(std::move(value)) {}
};

template<typename Is, typename ...T>
class shape {
  static_assert(std::is_same<Is, void>{}, "Bad one mapped");
};

template<size_t ...Is, typename ...T>
class shape<std::index_sequence<Is...>, T...> : public shape_node<Is, T> ... {
  template<size_t tag, typename T1>
  static decltype(auto) get_node_impl(shape_node<tag, T1> *t) { return *t; }
  template<size_t tag, typename T1>
  static decltype(auto) get_node_impl(const shape_node<tag, T1> *t) { return *t; }
  template<size_t tag>
  decltype(auto) get_node() { return get_node_impl<tag>(this); }
  template<size_t tag>
  decltype(auto) get_node() const { return get_node_impl<tag>(this); }

  template<size_t tag, typename T2>
  void set(const shape_node<tag, T2> &other) {
    get_node<tag>().value = other.value;
  }
  template<size_t tag, typename T2>
  void set(shape_node<tag, T2> &&other) {
    get_node<tag>().value = std::move(other.value);
  }

  template<bool eq_sizes, typename ...Args>
  class is_constructible_helper;

  template<typename ...Args>
  class is_constructible_helper<true, Args...> : public std::integral_constant<bool, vk::all_of_equal(true, std::is_constructible<T, Args>{}...)> {
  };

  template<typename ...Args>
  class is_constructible_helper<false, Args...> : public std::false_type {
  };

  template<typename ...Args>
  class is_constructible : public is_constructible_helper<sizeof...(Args) == sizeof...(Is), Args...> {
  };

  template<size_t ...Is2>
  using enable_if_sequence_is_subset = std::enable_if_t<vk::all_of_equal(true, vk::any_of_equal(Is2, Is...)...)>;

public:

  template<typename ...Args, typename = std::enable_if_t<is_constructible<Args...>{}>>
  explicit shape(Args &&...args) :
    shape_node<Is, T>(std::forward<Args>(args))... {}

  template<size_t tag>
  auto &get() { return get_node<tag>().value; }
  template<size_t tag>
  const auto &get() const { return get_node<tag>().value; }

  shape() = default;
  shape(const shape &) = default;
  shape(shape &&) noexcept = default;
  shape &operator=(const shape &) = default;
  shape &operator=(shape &&) noexcept = default;

  // TODO: not SFINAE-friendly
  template<size_t ...Is2, typename ...T2, typename = enable_if_sequence_is_subset<Is2...>>
  explicit shape(shape<std::index_sequence<Is2...>, T2...> &&other) noexcept :
    shape_node<Is, T>()... {
    std::initializer_list<int>{(set(std::forward<shape_node<Is2, T2>>(other)), 0)...};
  }

  template<size_t ...Is2, typename ...T2, typename = enable_if_sequence_is_subset<Is2...>>
  shape(const shape<std::index_sequence<Is2...>, T2...> &other) noexcept :
    shape_node<Is, T>()... {
    std::initializer_list<int>{(set(static_cast<const shape_node<Is2, T2> &>(other)), 0)...};
  }

  template<size_t ...Is2, typename ...T2, typename = enable_if_sequence_is_subset<Is2...>>
  shape &operator=(shape<std::index_sequence<Is2...>, T2...> other) {
    shape nv(std::move(other));
    std::swap(nv, *this);
    return *this;
  }
};

template<size_t ...Is, typename ...T>
inline bool f$boolval(const shape<std::index_sequence<Is...>, T...> &) {
  return true;
}
