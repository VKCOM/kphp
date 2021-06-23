#include <gtest/gtest.h>

#include "compiler/utils/ifimap.h"

TEST(ifimap_test, test_overflow) {
  IfiMap<1, ifi_is_array> m;

  m.insert("x", ifi_is_object);
  ASSERT_EQ(1, m.size()) << "size after first insert";

  m.insert("y", ifi_is_object);
  ASSERT_EQ(1, m.size()) << "size after the second insert";
}

TEST(ifimap_test, test_update) {
  IfiMap<4, ifi_is_array> m;

  m.insert("x", ifi_is_object);
  m.insert("y", ifi_is_bool);
  ASSERT_EQ(2, m.size()) << "size after inserts";
  ASSERT_EQ(ifi_is_object, m.get("x")) << "before update x value";
  ASSERT_EQ(ifi_is_bool, m.get("y")) << "before update y value";

  m.update_all(ifi_is_null);
  ASSERT_EQ(2, m.size()) << "size after update";
  ASSERT_EQ(ifi_is_null, m.get("x")) << "after update x value";
  ASSERT_EQ(ifi_is_null, m.get("y")) << "after update y value";
}

TEST(ifimap_test, test_insert_get_default) {
  IfiMap<8, ifi_is_array> is_array;
  ASSERT_EQ(ifi_is_array, is_array.get("non-existing")) << "non-existing key for is_array";
  ASSERT_EQ(0, is_array.size()) << "size after the non-existing key lookup";

  IfiMap<16> is_any_type;
  ASSERT_EQ(ifi_any_type, is_any_type.get("non-existing")) << "non-existing key for any_type";
  ASSERT_EQ(0, is_any_type.size()) << "size after the non-existing key lookup";
}

TEST(ifimap_test, test_insert_get_update) {
  vk::string_view shared_key = "shared";

  IfiMap<8> m;

  m.insert(shared_key, ifi_is_bool);
  for (int i = 0; i < 5; i++) {
    ASSERT_EQ(ifi_is_bool, m.get(shared_key)) << "lookup is_bool";
  }

  m.insert(shared_key, ifi_is_numeric);
  ASSERT_EQ(ifi_is_numeric, m.get(shared_key)) << "lookup is_numeric";

  ASSERT_EQ(1, m.size()) << "size after the update";

  vk::string_view key2 = "k2";
  m.insert(key2, ifi_is_object);
  ASSERT_EQ(ifi_is_object, m.get(key2)) << "lookup is_object";

  m.insert(shared_key, static_cast<is_func_id_t>(m.get(shared_key) | ifi_is_bool));
  ASSERT_EQ(ifi_is_numeric | ifi_is_bool, m.get(shared_key)) << "lookup is_bool|is_numeric";
  ASSERT_EQ(ifi_is_object, m.get(key2)) << "lookup is_object";

  ASSERT_EQ(2, m.size()) << "size after the inserts";
}

TEST(ifimap_test, test_copying) {
  vk::string_view shared_key = "shared";

  auto modify_map_copy = [=](IfiMap<4> m) {
    m.insert(shared_key, ifi_isset);
    m.insert(shared_key, ifi_is_array);
  };

  IfiMap<4> m;
  auto m2 = m;

  ASSERT_EQ(0, m.size()) << "zero map sizes";
  ASSERT_EQ(0, m2.size()) << "zero map sizes";

  vk::string_view key1 = "k1";
  m.insert(key1, ifi_is_string);

  ASSERT_EQ(1, m.size()) << "map size after the insert";
  ASSERT_EQ(0, m2.size()) << "map copy size";

  vk::string_view key2 = "k2";
  m2.insert(key2, ifi_is_array);

  modify_map_copy(m);
  modify_map_copy(m2);

  ASSERT_EQ(1, m.size()) << "map size";
  ASSERT_EQ(1, m2.size()) << "map copy size after the insert";
}
