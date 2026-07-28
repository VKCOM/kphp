#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <iterator>
#include <memory>
#include <numeric>
#include <vector>

#include "runtime-common/core/std/intrusive-list.h"

namespace {

template<class List>
auto values(const List& l) -> std::vector<typename List::value_type> {
  std::vector<typename List::value_type> v;
  for (const auto& x : l) {
    v.push_back(x);
  }

  return v;
}

template<class List>
auto reversed_values(List& l) -> std::vector<typename List::value_type> {
  std::vector<typename List::value_type> v;
  for (auto it = l.rbegin(); it != l.rend(); ++it) {
    v.push_back(*it);
  }

  return v;
}

} // namespace

TEST(intrusive_list_node_base, default_is_not_linked) {
  kphp::stl::intrusive::details::list_node_base b;

  ASSERT_FALSE(b.is_linked());
}

TEST(intrusive_list_node_base, unlink_on_unlinked_is_safe) {
  kphp::stl::intrusive::details::list_node_base b;
  b.unlink();
  b.unlink();

  ASSERT_FALSE(b.is_linked());
}

TEST(intrusive_list_basic, empty_list) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;

  ASSERT_TRUE(l.empty());
  ASSERT_EQ(l.size(), 0);
  ASSERT_EQ(l.begin(), l.end());
  ASSERT_EQ(l.cbegin(), l.cend());
  ASSERT_EQ(l.rbegin(), l.rend());
  ASSERT_EQ(l.crbegin(), l.crend());
}

TEST(intrusive_list_basic, push_back_single) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> n{42};

  l.push_back(n);

  ASSERT_FALSE(l.empty());
  ASSERT_EQ(l.size(), 1);
  ASSERT_EQ(l.front(), 42);
  ASSERT_EQ(l.back(), 42);
  ASSERT_EQ(std::addressof(l.front()), std::addressof(l.back()));
}

TEST(intrusive_list_basic, push_back_keeps_order) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  std::array<kphp::stl::intrusive::list_node<int>, 3> ns{kphp::stl::intrusive::list_node<int>{1}, kphp::stl::intrusive::list_node<int>{2},
                                                         kphp::stl::intrusive::list_node<int>{3}};
  for (auto& n : ns) {
    l.push_back(n);
  }

  ASSERT_EQ(l.size(), 3);
  ASSERT_EQ(values(l), (std::vector<int>{1, 2, 3}));
}

TEST(intrusive_list_basic, push_front_single) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> n{42};

  l.push_front(n);

  ASSERT_FALSE(l.empty());
  ASSERT_EQ(l.size(), 1);
  ASSERT_EQ(l.front(), 42);
  ASSERT_EQ(l.back(), 42);
  ASSERT_EQ(std::addressof(l.front()), std::addressof(l.back()));
}

TEST(intrusive_list_basic, push_front_reverses_order) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  std::array<kphp::stl::intrusive::list_node<int>, 3> ns{kphp::stl::intrusive::list_node<int>{1}, kphp::stl::intrusive::list_node<int>{2},
                                                         kphp::stl::intrusive::list_node<int>{3}};
  for (auto& n : ns) {
    l.push_front(n);
  }

  ASSERT_EQ(l.size(), 3);
  ASSERT_EQ(values(l), (std::vector<int>{3, 2, 1}));
}

TEST(intrusive_list_basic, mixed_push_front_and_back) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> a{1}, b{2}, c{3}, d{4};
  l.push_back(b);  // 2
  l.push_front(a); // 1 2
  l.push_back(c);  // 1 2 3
  l.push_front(d); // 4 1 2 3

  ASSERT_EQ(values(l), (std::vector<int>{4, 1, 2, 3}));
}

TEST(intrusive_list_basic, front_back_are_mutable_references) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> a{1}, b{2};
  l.push_back(a);
  l.push_back(b);
  l.front() = 10;
  l.back() = 20;

  ASSERT_EQ(values(l), (std::vector<int>{10, 20}));
  ASSERT_EQ(a.value(), 10);
  ASSERT_EQ(b.value(), 20);
}

TEST(intrusive_list_iteration, forward) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  std::array<kphp::stl::intrusive::list_node<int>, 4> ns{kphp::stl::intrusive::list_node<int>{1}, kphp::stl::intrusive::list_node<int>{2},
                                                         kphp::stl::intrusive::list_node<int>{3}, kphp::stl::intrusive::list_node<int>{4}};
  for (auto& n : ns) {
    l.push_back(n);
  }

  ASSERT_EQ(values(l), (std::vector<int>{1, 2, 3, 4}));
}

TEST(intrusive_list_iteration, reverse) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  std::array<kphp::stl::intrusive::list_node<int>, 4> ns{kphp::stl::intrusive::list_node<int>{1}, kphp::stl::intrusive::list_node<int>{2},
                                                         kphp::stl::intrusive::list_node<int>{3}, kphp::stl::intrusive::list_node<int>{4}};
  for (auto& n : ns) {
    l.push_back(n);
  }

  ASSERT_EQ(reversed_values(l), (std::vector<int>{4, 3, 2, 1}));
}

TEST(intrusive_list_iteration, bidirectional_increment_decrement) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  std::array<kphp::stl::intrusive::list_node<int>, 3> ns{kphp::stl::intrusive::list_node<int>{1}, kphp::stl::intrusive::list_node<int>{2},
                                                         kphp::stl::intrusive::list_node<int>{3}};
  for (auto& n : ns) {
    l.push_back(n);
  }
  auto it = l.begin();

  ASSERT_EQ(*it, 1);
  ASSERT_EQ(*(++it), 2);
  ASSERT_EQ(*(it++), 2);
  ASSERT_EQ(*it, 3);
  ASSERT_EQ(*(--it), 2);
  ASSERT_EQ(*(it--), 2);
  ASSERT_EQ(*it, 1);

  auto last = l.end();
  --last;

  ASSERT_EQ(*last, 3);
}

TEST(intrusive_list_iteration, const_iteration) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  std::array<kphp::stl::intrusive::list_node<int>, 3> ns{kphp::stl::intrusive::list_node<int>{7}, kphp::stl::intrusive::list_node<int>{8},
                                                         kphp::stl::intrusive::list_node<int>{9}};
  for (auto& n : ns) {
    l.push_back(n);
  }
  const kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>>& cl = l;

  ASSERT_EQ(values(cl), (std::vector<int>{7, 8, 9}));

  std::vector<int> collected;
  for (auto it = cl.cbegin(); it != cl.cend(); ++it) {
    collected.push_back(*it);
  }

  ASSERT_EQ(collected, (std::vector<int>{7, 8, 9}));

  std::vector<int> reversed_collected;
  for (auto it = cl.crbegin(); it != cl.crend(); ++it) {
    reversed_collected.push_back(*it);
  }

  ASSERT_EQ(reversed_collected, (std::vector<int>{9, 8, 7}));
}

TEST(intrusive_list_iteration, iterator_converts_to_const_and_compares) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> a{1};
  l.push_back(a);

  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>>::iterator it = l.begin();
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>>::const_iterator cit = it; // implicit non-const -> const conversion

  ASSERT_EQ(it, cit);
  ASSERT_EQ(cit, it);
  ASSERT_FALSE(it != cit);
}

TEST(intrusive_list_iteration, works_with_std_algorithms) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  std::array<kphp::stl::intrusive::list_node<int>, 5> ns{kphp::stl::intrusive::list_node<int>{1}, kphp::stl::intrusive::list_node<int>{2},
                                                         kphp::stl::intrusive::list_node<int>{3}, kphp::stl::intrusive::list_node<int>{4},
                                                         kphp::stl::intrusive::list_node<int>{5}};
  for (auto& n : ns) {
    l.push_back(n);
  }

  ASSERT_EQ(std::distance(l.begin(), l.end()), 5);

  auto found = std::find(l.begin(), l.end(), 3);

  ASSERT_NE(found, l.end());
  ASSERT_EQ(*found, 3);

  ASSERT_EQ(std::find(l.begin(), l.end(), 42), l.end());
  ASSERT_EQ(std::accumulate(l.begin(), l.end(), 0), 15);
}

TEST(intrusive_list_insert, at_begin) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> a{2}, b{1};
  l.push_back(a);
  auto it = l.insert(l.begin(), b);

  ASSERT_EQ(*it, 1);
  ASSERT_EQ(values(l), (std::vector<int>{1, 2}));
}

TEST(intrusive_list_insert, at_end) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> a{1}, b{2};
  l.insert(l.end(), a);
  l.insert(l.end(), b);

  ASSERT_EQ(values(l), (std::vector<int>{1, 2}));
}

TEST(intrusive_list_insert, in_middle_returns_iterator_to_new_node) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> a{1}, c{3}, b{2};
  l.push_back(a);
  l.push_back(c);
  auto pos = std::next(l.begin()); // points at 3
  auto it = l.insert(pos, b);

  ASSERT_EQ(*it, 2);
  ASSERT_EQ(values(l), (std::vector<int>{1, 2, 3}));
}

TEST(intrusive_list_insert, same_node_at_its_own_position_is_noop) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> a{1}, b{2};
  l.push_back(a);
  l.push_back(b);
  // Inserting node 'a' right before itself must not corrupt the list.
  auto it = l.insert(l.begin(), a);

  ASSERT_EQ(*it, 1);
  ASSERT_EQ(values(l), (std::vector<int>{1, 2}));
}

TEST(intrusive_list_insert, relinks_node_moving_it_from_another_list) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> src;
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> dst;
  kphp::stl::intrusive::list_node<int> a{1}, b{2}, c{3};
  src.push_back(a);
  src.push_back(b);
  src.push_back(c);

  // Moving 'b' into dst removes it from src (insert unlinks first).
  dst.push_back(b);
  ASSERT_EQ(values(src), (std::vector<int>{1, 3}));
  ASSERT_EQ(values(dst), (std::vector<int>{2}));
}

TEST(intrusive_list_insert, moves_node_within_same_list) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> a{1}, b{2}, c{3};
  l.push_back(a);
  l.push_back(b);
  l.push_back(c);
  // Re-insert front node before end -> moves it to the back.
  l.insert(l.end(), a);

  ASSERT_EQ(values(l), (std::vector<int>{2, 3, 1}));
  ASSERT_EQ(l.size(), 3);
}

TEST(intrusive_list_erase, single_returns_next) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> a{1}, b{2}, c{3};
  l.push_back(a);
  l.push_back(b);
  l.push_back(c);
  auto next = l.erase(std::next(l.begin())); // erase 2

  ASSERT_EQ(*next, 3);
  ASSERT_EQ(values(l), (std::vector<int>{1, 3}));
}

TEST(intrusive_list_erase, front) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> a{1}, b{2};
  l.push_back(a);
  l.push_back(b);
  auto next = l.erase(l.begin());

  ASSERT_EQ(*next, 2);
  ASSERT_EQ(values(l), (std::vector<int>{2}));
}

TEST(intrusive_list_erase, end) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> a{1}, b{2};
  l.push_back(a);
  l.push_back(b);
  auto next = l.erase(std::prev(l.end()));

  ASSERT_EQ(next, l.end());
  ASSERT_EQ(values(l), (std::vector<int>{1}));
}

TEST(intrusive_list_erase, range) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  std::array<kphp::stl::intrusive::list_node<int>, 5> ns{kphp::stl::intrusive::list_node<int>{1}, kphp::stl::intrusive::list_node<int>{2},
                                                         kphp::stl::intrusive::list_node<int>{3}, kphp::stl::intrusive::list_node<int>{4},
                                                         kphp::stl::intrusive::list_node<int>{5}};
  for (auto& n : ns) {
    l.push_back(n);
  }
  // erase [2, 4) -> removes 2 and 3
  auto first = std::next(l.begin());
  auto last = std::next(l.begin(), 3);
  auto it = l.erase(first, last);

  ASSERT_EQ(*it, 4);
  ASSERT_EQ(values(l), (std::vector<int>{1, 4, 5}));
}

TEST(intrusive_list_erase, empty_range_is_noop) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> a{1}, b{2};
  l.push_back(a);
  l.push_back(b);
  auto it = l.erase(l.begin(), l.begin());

  ASSERT_EQ(*it, 1);
  ASSERT_EQ(values(l), (std::vector<int>{1, 2}));
}

TEST(intrusive_list_erase, all_makes_empty) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  std::array<kphp::stl::intrusive::list_node<int>, 3> ns{kphp::stl::intrusive::list_node<int>{1}, kphp::stl::intrusive::list_node<int>{2},
                                                         kphp::stl::intrusive::list_node<int>{3}};
  for (auto& n : ns) {
    l.push_back(n);
  }
  auto it = l.erase(l.begin(), l.end());

  ASSERT_EQ(it, l.end());
  ASSERT_TRUE(l.empty());
  ASSERT_EQ(l.size(), 0);
}

TEST(intrusive_list_erase, node_is_reusable_after_erase) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l1;
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l2;
  kphp::stl::intrusive::list_node<int> a{1};
  l1.push_back(a);
  l1.erase(l1.begin());

  ASSERT_TRUE(l1.empty());

  // The same node object can be linked into another list afterwards.
  l2.push_back(a);

  ASSERT_EQ(values(l2), (std::vector<int>{1}));
}

TEST(intrusive_list_pop, pop_back) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> a{1}, b{2}, c{3};
  l.push_back(a);
  l.push_back(b);
  l.push_back(c);
  l.pop_back();

  ASSERT_EQ(values(l), (std::vector<int>{1, 2}));

  l.pop_back();

  ASSERT_EQ(values(l), (std::vector<int>{1}));
}

TEST(intrusive_list_pop, pop_front) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> a{1}, b{2}, c{3};
  l.push_back(a);
  l.push_back(b);
  l.push_back(c);
  l.pop_front();

  ASSERT_EQ(values(l), (std::vector<int>{2, 3}));
  l.pop_front();

  ASSERT_EQ(values(l), (std::vector<int>{3}));
}

TEST(intrusive_list_pop, pop_until_empty) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> a{1}, b{2};
  l.push_back(a);
  l.push_back(b);
  l.pop_front();
  l.pop_back();

  ASSERT_TRUE(l.empty());
}

TEST(intrusive_list_clear, makes_list_empty) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  std::array<kphp::stl::intrusive::list_node<int>, 3> ns{kphp::stl::intrusive::list_node<int>{1}, kphp::stl::intrusive::list_node<int>{2},
                                                         kphp::stl::intrusive::list_node<int>{3}};
  for (auto& n : ns) {
    l.push_back(n);
  }
  l.clear();

  ASSERT_TRUE(l.empty());
  ASSERT_EQ(l.size(), 0);
  ASSERT_EQ(l.begin(), l.end());
}

TEST(intrusive_list_clear, list_is_reusable_after_clear) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  std::array<kphp::stl::intrusive::list_node<int>, 3> ns{kphp::stl::intrusive::list_node<int>{1}, kphp::stl::intrusive::list_node<int>{2},
                                                         kphp::stl::intrusive::list_node<int>{3}};
  for (auto& n : ns) {
    l.push_back(n);
  }
  l.clear();
  kphp::stl::intrusive::list_node<int> x{10}, y{20};
  l.push_back(x);
  l.push_back(y);

  ASSERT_EQ(values(l), (std::vector<int>{10, 20}));
}

TEST(intrusive_list_swap, two_non_empty) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> a;
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> b;
  kphp::stl::intrusive::list_node<int> a1{1}, a2{2};
  kphp::stl::intrusive::list_node<int> b1{3}, b2{4}, b3{5};
  a.push_back(a1);
  a.push_back(a2);
  b.push_back(b1);
  b.push_back(b2);
  b.push_back(b3);

  a.swap(b);

  ASSERT_EQ(values(a), (std::vector<int>{3, 4, 5}));
  ASSERT_EQ(values(b), (std::vector<int>{1, 2}));
}

TEST(intrusive_list_swap, empty_with_non_empty) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> a;
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> b;
  kphp::stl::intrusive::list_node<int> b1{1}, b2{2};
  b.push_back(b1);
  b.push_back(b2);

  a.swap(b);

  ASSERT_EQ(values(a), (std::vector<int>{1, 2}));
  ASSERT_TRUE(b.empty());

  a.swap(b);

  ASSERT_TRUE(a.empty());
  ASSERT_EQ(values(b), (std::vector<int>{1, 2}));
}

TEST(intrusive_list_swap, both_empty) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> a;
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> b;
  a.swap(b);

  ASSERT_TRUE(a.empty());
  ASSERT_TRUE(b.empty());
}

TEST(intrusive_list_swap, free_function) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> a;
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> b;
  kphp::stl::intrusive::list_node<int> a1{1};
  kphp::stl::intrusive::list_node<int> b1{2}, b2{3};
  a.push_back(a1);
  b.push_back(b1);
  b.push_back(b2);

  swap(a, b);

  ASSERT_EQ(values(a), (std::vector<int>{2, 3}));
  ASSERT_EQ(values(b), (std::vector<int>{1}));
}

TEST(intrusive_list_swap, swap_with_self) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> a;
  kphp::stl::intrusive::list_node<int> a1{1}, a2{2};

  a.swap(a);

  ASSERT_EQ(values(a), (std::vector<int>{1, 2}));

  swap(a, a);

  ASSERT_EQ(values(a), (std::vector<int>{1, 2}));
}

TEST(intrusive_list_splice, whole_list_at_end) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> a;
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> b;
  kphp::stl::intrusive::list_node<int> a1{1}, a2{2}, a3{3};
  kphp::stl::intrusive::list_node<int> b1{4}, b2{5};
  a.push_back(a1);
  a.push_back(a2);
  a.push_back(a3);
  b.push_back(b1);
  b.push_back(b2);

  b.splice(b.end(), a);

  ASSERT_EQ(values(b), (std::vector<int>{4, 5, 1, 2, 3}));
  ASSERT_TRUE(a.empty());
}

TEST(intrusive_list_splice, whole_list_at_begin) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> a;
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> b;
  kphp::stl::intrusive::list_node<int> a1{1}, a2{2};
  kphp::stl::intrusive::list_node<int> b1{3}, b2{4};
  a.push_back(a1);
  a.push_back(a2);
  b.push_back(b1);
  b.push_back(b2);

  b.splice(b.begin(), a);

  ASSERT_EQ(values(b), (std::vector<int>{1, 2, 3, 4}));
  ASSERT_TRUE(a.empty());
}

TEST(intrusive_list_splice, whole_list_in_middle) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> a;
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> b;
  kphp::stl::intrusive::list_node<int> a1{1}, a2{2};
  kphp::stl::intrusive::list_node<int> b1{3}, b2{4};
  a.push_back(a1);
  a.push_back(a2);
  b.push_back(b1);
  b.push_back(b2);

  b.splice(std::next(b.begin()), a); // between 3 and 4

  ASSERT_EQ(values(b), (std::vector<int>{3, 1, 2, 4}));
  ASSERT_TRUE(a.empty());
}

TEST(intrusive_list_splice, single_element) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> a;
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> b;
  kphp::stl::intrusive::list_node<int> a1{1}, a2{2}, a3{3};
  a.push_back(a1);
  a.push_back(a2);
  a.push_back(a3);

  b.splice(b.end(), a, std::next(a.begin())); // move `2`

  ASSERT_EQ(values(b), (std::vector<int>{2}));
  ASSERT_EQ(values(a), (std::vector<int>{1, 3}));
}

TEST(intrusive_list_splice, sub_range) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> a;
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> b;
  std::array<kphp::stl::intrusive::list_node<int>, 5> ns{kphp::stl::intrusive::list_node<int>{1}, kphp::stl::intrusive::list_node<int>{2},
                                                         kphp::stl::intrusive::list_node<int>{3}, kphp::stl::intrusive::list_node<int>{4},
                                                         kphp::stl::intrusive::list_node<int>{5}};
  for (auto& n : ns) {
    a.push_back(n);
  }
  // move [2, 4) -> nodes 2 and 3
  auto first = std::next(a.begin());
  auto last = std::next(a.begin(), 3);
  b.splice(b.end(), a, first, last);

  ASSERT_EQ(values(b), (std::vector<int>{2, 3}));
  ASSERT_EQ(values(a), (std::vector<int>{1, 4, 5}));
}

TEST(intrusive_list_splice, empty_other_is_noop) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> a;
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> b;
  kphp::stl::intrusive::list_node<int> b1{1}, b2{2};
  b.push_back(b1);
  b.push_back(b2);

  b.splice(b.begin(), a);

  ASSERT_EQ(values(b), (std::vector<int>{1, 2}));
  ASSERT_TRUE(a.empty());
}

TEST(intrusive_list_splice, within_same_list_moves_node) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> a{1}, b{2}, c{3};
  l.push_back(a);
  l.push_back(b);
  l.push_back(c);
  // move node `2` to the front
  l.splice(l.begin(), l, std::next(l.begin()));

  ASSERT_EQ(values(l), (std::vector<int>{2, 1, 3}));
  ASSERT_EQ(l.size(), 3);
}

TEST(intrusive_list_splice, self_splice_to_same_position_is_noop) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> a{1}, b{2};
  l.push_back(a);
  l.push_back(b);
  // splicing the first node to the position right after it (== its own place) is a noop
  l.splice(std::next(l.begin()), l, l.begin());

  ASSERT_EQ(values(l), (std::vector<int>{1, 2}));

  // splicing the first node to the position right before it (== its own place) is a noop
  l.splice(l.begin(), l, l.begin());

  ASSERT_EQ(values(l), (std::vector<int>{1, 2}));
}

TEST(intrusive_list_splice, rvalue_overloads) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> a;
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> b;
  kphp::stl::intrusive::list_node<int> a1{1}, a2{2};
  a.push_back(a1);
  a.push_back(a2);

  b.splice(b.end(), std::move(a));

  ASSERT_EQ(values(b), (std::vector<int>{1, 2}));
  ASSERT_TRUE(a.empty());

  a.splice(a.begin(), std::move(b), b.begin());

  ASSERT_EQ(values(a), (std::vector<int>{1}));
  ASSERT_EQ(values(b), (std::vector<int>{2}));

  a.splice(a.begin(), std::move(b), b.begin(), std::next(b.begin()));

  ASSERT_EQ(values(a), (std::vector<int>{2, 1}));
  ASSERT_TRUE(b.empty());
}

TEST(intrusive_list_move, move_construct_transfers_nodes) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> src;
  std::array<kphp::stl::intrusive::list_node<int>, 3> ns{kphp::stl::intrusive::list_node<int>{1}, kphp::stl::intrusive::list_node<int>{2},
                                                         kphp::stl::intrusive::list_node<int>{3}};
  for (auto& n : ns) {
    src.push_back(n);
  }
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> dst{std::move(src)};

  ASSERT_EQ(values(dst), (std::vector<int>{1, 2, 3}));
  ASSERT_TRUE(src.empty());
}

TEST(intrusive_list_move, move_assign_into_empty) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> src;
  kphp::stl::intrusive::list_node<int> a{1}, b{2};
  src.push_back(a);
  src.push_back(b);

  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> dst;
  dst = std::move(src);

  ASSERT_EQ(values(dst), (std::vector<int>{1, 2}));
  ASSERT_TRUE(src.empty());
}

TEST(intrusive_list_move, move_assign) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> src;
  kphp::stl::intrusive::list_node<int> a{1}, b{2}, c{3}, d{4};
  src.push_back(a);
  src.push_back(b);

  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> dst;
  dst.push_back(c);
  dst.push_back(d);
  dst = std::move(src);

  ASSERT_EQ(values(dst), (std::vector<int>{1, 2}));
  ASSERT_TRUE(src.empty());
}

namespace {

struct tag_a {};
struct tag_b {};

} // namespace

TEST(intrusive_list_tags, node_lives_in_two_lists_at_once) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int, tag_a, tag_b>, tag_a> la;
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int, tag_a, tag_b>, tag_b> lb;
  kphp::stl::intrusive::list_node<int, tag_a, tag_b> n1{1}, n2{2}, n3{3};

  la.push_back(n1);
  la.push_back(n2);
  la.push_back(n3);

  lb.push_back(n3);
  lb.push_back(n2);
  lb.push_back(n1);

  ASSERT_EQ(values(la), (std::vector<int>{1, 2, 3}));
  ASSERT_EQ(values(lb), (std::vector<int>{3, 2, 1}));
}

TEST(intrusive_list_tags, erase_from_one_list_keeps_the_other) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int, tag_a, tag_b>, tag_a> la;
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int, tag_a, tag_b>, tag_b> lb;
  kphp::stl::intrusive::list_node<int, tag_a, tag_b> n1{1}, n2{2}, n3{3};

  la.push_back(n1);
  la.push_back(n2);
  la.push_back(n3);
  lb.push_back(n1);
  lb.push_back(n2);
  lb.push_back(n3);

  la.erase(la.begin()); // remove n1 from `la` only

  ASSERT_EQ(values(la), (std::vector<int>{2, 3}));
  ASSERT_EQ(values(lb), (std::vector<int>{1, 2, 3}));
}

TEST(intrusive_list_tags, mutation_through_one_tag_visible_via_other) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int, tag_a, tag_b>, tag_a> la;
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int, tag_a, tag_b>, tag_b> lb;
  kphp::stl::intrusive::list_node<int, tag_a, tag_b> n{5};
  la.push_back(n);
  lb.push_back(n);

  la.front() = 99;

  ASSERT_EQ(lb.front(), 99);
}

TEST(intrusive_list_value, stores_move_only_value_type) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<std::unique_ptr<int>>> l;
  kphp::stl::intrusive::list_node<std::unique_ptr<int>> a{std::make_unique<int>(1)};
  kphp::stl::intrusive::list_node<std::unique_ptr<int>> b{std::make_unique<int>(2)};
  l.push_back(a);
  l.push_back(b);

  auto it = l.begin();

  ASSERT_EQ(**it, 1);

  ++it;

  ASSERT_EQ(**it, 2);
}

TEST(intrusive_list_value, make_list_node_helper) {
  auto n = kphp::stl::intrusive::make_list_node(123);
  static_assert(std::is_same_v<decltype(n)::value_type, int>);
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  l.push_back(n);

  ASSERT_EQ(l.front(), 123);
}

TEST(intrusive_list_iterator_to, single_node) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> a{42};
  l.push_back(a);

  auto it = l.iterator_to(a);

  ASSERT_EQ(it, l.begin());
  ASSERT_EQ(*it, 42);
}

TEST(intrusive_list_iterator_to, points_at_correct_position_in_middle) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> a{1}, b{2}, c{3};
  l.push_back(a);
  l.push_back(b);
  l.push_back(c);

  auto it = l.iterator_to(b);

  ASSERT_EQ(*it, 2);
  ASSERT_EQ(it, std::next(l.begin()));
  ASSERT_EQ(*std::prev(it), 1);
  ASSERT_EQ(*std::next(it), 3);
}

TEST(intrusive_list_iterator_to, works_for_front_and_back) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> a{1}, b{2}, c{3};
  l.push_back(a);
  l.push_back(b);
  l.push_back(c);

  ASSERT_EQ(l.iterator_to(a), l.begin());
  ASSERT_EQ(l.iterator_to(c), std::prev(l.end()));
}

TEST(intrusive_list_iterator_to, const_overload) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> a{1}, b{2};
  l.push_back(a);
  l.push_back(b);
  const auto& cl = l;

  auto cit = cl.iterator_to(b);

  ASSERT_EQ(*cit, 2);
  ASSERT_EQ(cit, std::next(cl.cbegin()));
}

TEST(intrusive_list_iterator_to, mutating_through_returned_iterator_is_visible_via_node) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> a{1};
  l.push_back(a);

  auto it = l.iterator_to(a);
  *it = 100;

  ASSERT_EQ(a.value(), 100);
}

TEST(intrusive_list_iterator_to, remains_valid_after_owning_node_is_relocated) {
  kphp::stl::intrusive::list<kphp::stl::intrusive::list_node<int>> l;
  kphp::stl::intrusive::list_node<int> a{1}, b{2};
  l.push_back(a);
  l.push_back(b);

  auto relocated = std::make_unique<kphp::stl::intrusive::list_node<int>>(std::move(a));
  auto it = l.iterator_to(*relocated);

  ASSERT_EQ(*it, 1);
  ASSERT_EQ(it, l.begin());
  ASSERT_EQ(values(l), (std::vector<int>{1, 2}));
}
