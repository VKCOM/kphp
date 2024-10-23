#include <gtest/gtest.h>

#include "runtime-common/runtime-core/runtime-core.h"
#include "runtime/array_functions.h"

class ArrayIntStringKeysCollision : public testing::Test {
protected:
  const string string_key{"my_key"};
  const int64_t int_key{string_hash(string_key.c_str(), string_key.size())};
};

TEST_F(ArrayIntStringKeysCollision, string_int_collision) {
  array<string> arr;
  ASSERT_FALSE(arr.find_value(string_key));
  ASSERT_FALSE(arr.find_value(int_key));

  arr.set_value(string_key, string{"string_key"});
  ASSERT_TRUE(arr.find_value(string_key));
  ASSERT_EQ(arr.get_value(string_key), string{"string_key"});
  ASSERT_FALSE(arr.find_value(int_key));
  ASSERT_EQ(arr.get_value(int_key), string{});

  arr.set_value(int_key, string{"int_key"});
  ASSERT_TRUE(arr.find_value(string_key));
  ASSERT_EQ(arr.get_value(string_key), string{"string_key"});
  ASSERT_TRUE(arr.find_value(int_key));
  ASSERT_EQ(arr.get_value(int_key), string{"int_key"});
}

TEST_F(ArrayIntStringKeysCollision, int_string_collision) {
  array<string> arr;
  ASSERT_FALSE(arr.find_value(string_key));
  ASSERT_FALSE(arr.find_value(int_key));

  arr.set_value(int_key, string{"int_key"});
  ASSERT_TRUE(arr.find_value(int_key));
  ASSERT_EQ(arr.get_value(int_key), string{"int_key"});
  ASSERT_FALSE(arr.find_value(string_key));
  ASSERT_EQ(arr.get_value(string_key), string{});

  arr.set_value(string_key, string{"string_key"});
  ASSERT_TRUE(arr.find_value(int_key));
  ASSERT_EQ(arr.get_value(int_key), string{"int_key"});
  ASSERT_TRUE(arr.find_value(string_key));
  ASSERT_EQ(arr.get_value(string_key), string{"string_key"});
}

TEST_F(ArrayIntStringKeysCollision, unset_int_string_collision) {
  array<string> arr;
  arr.set_value(string_key, string{"string_key"});
  arr.set_value(int_key, string{"int_key"});

  ASSERT_EQ(arr.unset(int_key), string{"int_key"});
  ASSERT_TRUE(arr.find_value(string_key));
  ASSERT_EQ(arr.get_value(string_key), string{"string_key"});
  ASSERT_FALSE(arr.find_value(int_key));
  ASSERT_EQ(arr.get_value(int_key), string{});

  ASSERT_EQ(arr.unset(string_key), string{"string_key"});
  ASSERT_FALSE(arr.find_value(string_key));
  ASSERT_EQ(arr.get_value(string_key), string{});
  ASSERT_FALSE(arr.find_value(int_key));
  ASSERT_EQ(arr.get_value(int_key), string{});
}

TEST_F(ArrayIntStringKeysCollision, unset_string_int_collision) {
  array<string> arr;
  arr.set_value(int_key, string{"int_key"});
  arr.set_value(string_key, string{"string_key"});

  ASSERT_EQ(arr.unset(string_key), string{"string_key"});
  ASSERT_FALSE(arr.find_value(string_key));
  ASSERT_EQ(arr.get_value(string_key), string{});
  ASSERT_TRUE(arr.find_value(int_key));
  ASSERT_EQ(arr.get_value(int_key), string{"int_key"});

  ASSERT_EQ(arr.unset(int_key), string{"int_key"});
  ASSERT_FALSE(arr.find_value(string_key));
  ASSERT_EQ(arr.get_value(string_key), string{});
  ASSERT_FALSE(arr.find_value(int_key));
  ASSERT_EQ(arr.get_value(int_key), string{});
}

TEST_F(ArrayIntStringKeysCollision, ksort_int_string_collision) {
  array<string> arr;
  arr.set_value(int_key, string{"int_key"});
  arr.set_value(string_key, string{"string_key"});

  f$ksort(arr);

  auto it = arr.begin();
  ASSERT_TRUE(it.is_string_key());
  ASSERT_EQ(it.get_string_key(), string_key);
  ++it;
  ASSERT_FALSE(it.is_string_key());
  ASSERT_EQ(it.get_int_key(), int_key);
}

TEST_F(ArrayIntStringKeysCollision, ksort_string_int_collision) {
  array<string> arr;
  arr.set_value(string_key, string{"string_key"});
  arr.set_value(int_key, string{"int_key"});

  f$ksort(arr);

  auto it = arr.begin();
  ASSERT_TRUE(it.is_string_key());
  ASSERT_EQ(it.get_string_key(), string_key);
  ++it;
  ASSERT_FALSE(it.is_string_key());
  ASSERT_EQ(it.get_int_key(), int_key);
}
