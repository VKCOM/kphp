#include <gtest/gtest.h>

#include "runtime-common/core/runtime-core.h"

TEST(array_test, find_no_mutate_in_empy_array) {
  array<int> arr;

  ASSERT_EQ(arr.find_no_mutate(1), arr.end());
  ASSERT_EQ(arr.find_no_mutate(string{}), arr.end());
  ASSERT_EQ(arr.find_no_mutate(string{"foo"}), arr.end());
  ASSERT_EQ(arr.find_no_mutate(mixed{}), arr.end());
  ASSERT_TRUE(arr.is_reference_counter(ExtraRefCnt::for_global_const));
}

TEST(array_test, find_no_mutate_in_array_vector) {
  auto arr = array<int>::create(0, 1, 2, 3, 4, 5, 6, 7, 8);
  const auto arr_copy = arr;

  ASSERT_TRUE(arr.is_vector());

  ASSERT_EQ(arr.find_no_mutate(14), arr.end());
  ASSERT_EQ(arr.find_no_mutate(string{"foo"}), arr.end());

  auto it = arr.find_no_mutate(string{"4"});
  ASSERT_NE(it, arr.end());
  ASSERT_FALSE(it.is_string_key());
  ASSERT_TRUE(equals(it.get_key(), mixed{4}));
  ASSERT_EQ(it.get_value(), 4);

  it = arr.find_no_mutate(mixed{6});
  ASSERT_NE(it, arr.end());
  ASSERT_FALSE(it.is_string_key());
  ASSERT_TRUE(equals(it.get_key(), mixed{6}));
  ASSERT_EQ(it.get_value(), 6);

  it = arr.find_no_mutate(8);
  ASSERT_NE(it, arr.end());
  ASSERT_FALSE(it.is_string_key());
  ASSERT_TRUE(equals(it.get_key(), mixed{8}));
  ASSERT_EQ(it.get_value(), 8);

  ASSERT_EQ(arr.get_reference_counter(), 2);
}

TEST(array_test, find_no_mutate_in_array_map) {
  array<int> arr{
    std::make_pair(mixed{string{"key_1"}}, 1),
    std::make_pair(mixed{string{"key_2"}}, 2),
    std::make_pair(mixed{3}, 3),
    std::make_pair(mixed{string{"4"}}, 4)
  };
  const auto arr_copy = arr;

  ASSERT_FALSE(arr.is_vector());

  ASSERT_EQ(arr.find_no_mutate(1), arr.end());
  ASSERT_EQ(arr.find_no_mutate(string{"foo"}), arr.end());

  auto it = arr.find_no_mutate(string{"key_1"});
  ASSERT_NE(it, arr.end());
  ASSERT_TRUE(it.is_string_key());
  ASSERT_EQ(it.get_string_key(), string{"key_1"});
  ASSERT_EQ(it.get_value(), 1);

  it = arr.find_no_mutate(string{"3"});
  ASSERT_NE(it, arr.end());
  ASSERT_FALSE(it.is_string_key());
  ASSERT_TRUE(equals(it.get_key(), mixed{3}));
  ASSERT_EQ(it.get_value(), 3);

  it = arr.find_no_mutate(4);
  ASSERT_NE(it, arr.end());
  ASSERT_FALSE(it.is_string_key());
  ASSERT_TRUE(equals(it.get_key(), mixed{4}));
  ASSERT_EQ(it.get_value(), 4);

  ASSERT_EQ(arr.get_reference_counter(), 2);
}

TEST(array_test, test_is_equal_innter_pointer) {
  auto arr_vector = array<int>::create(0, 1, 2, 3, 4, 5, 6, 7, 8);
  array<int> arr_map{
    std::make_pair(mixed{string{"key_1"}}, 1),
    std::make_pair(mixed{3}, 3),
  };

  ASSERT_TRUE(arr_vector.is_vector());
  ASSERT_FALSE(arr_map.is_vector());

  ASSERT_TRUE(arr_vector.is_equal_inner_pointer(arr_vector));
  ASSERT_TRUE(arr_map.is_equal_inner_pointer(arr_map));

  ASSERT_FALSE(arr_vector.is_equal_inner_pointer(arr_map));
  ASSERT_FALSE(arr_map.is_equal_inner_pointer(arr_vector));
}

TEST(array_test, test_mutate_shared_vector) {
  auto arr = array<int>::create(0, 1, 2, 3, 4, 5, 6, 7, 8);
  ASSERT_TRUE(arr.is_vector());
  // no effect
  arr.mutate_if_shared();

  const auto arr_copy = arr;

  ASSERT_TRUE(arr.is_equal_inner_pointer(arr_copy));
  ASSERT_EQ(arr.get_reference_counter(), 2);

  arr.mutate_if_shared();
  ASSERT_EQ(arr.get_reference_counter(), 1);
  ASSERT_FALSE(arr.is_equal_inner_pointer(arr_copy));
  ASSERT_EQ(arr_copy.get_reference_counter(), 1);
  ASSERT_FALSE(arr_copy.is_equal_inner_pointer(arr));
}

TEST(array_test, test_mutate_shared_map) {
  array<int> arr{
    std::make_pair(mixed{string{"key_1"}}, 1),
    std::make_pair(mixed{3}, 3),
  };
  ASSERT_FALSE(arr.is_vector());
  // no effect
  arr.mutate_if_shared();

  const auto arr_copy = arr;

  ASSERT_TRUE(arr.is_equal_inner_pointer(arr_copy));
  ASSERT_EQ(arr.get_reference_counter(), 2);

  arr.mutate_if_shared();
  ASSERT_EQ(arr.get_reference_counter(), 1);
  ASSERT_FALSE(arr.is_equal_inner_pointer(arr_copy));
  ASSERT_EQ(arr_copy.get_reference_counter(), 1);
  ASSERT_FALSE(arr_copy.is_equal_inner_pointer(arr));
}
