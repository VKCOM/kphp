#include <gtest/gtest.h>

#include "server/confdata-binlog-events.h"

template<class T>
T *my_malloc(size_t additional_size) noexcept {
  return static_cast<T *>(std::malloc(sizeof(T) + additional_size));
}

TEST(confdata_binlog_events_test, test_delete_event) {
  const char key[] = "hello";
  auto *ev = my_malloc<lev_pmemcached_delete>(std::strlen(key) - 1);
  ev->type = LEV_PMEMCACHED_DELETE;
  ev->key_len = std::strlen(key);
  std::strcpy(ev->key, key);

  auto *confdata_ev = static_cast<lev_confdata_delete *>(ev);
  ASSERT_EQ(confdata_ev->get_extra_bytes(), std::strlen(key) - 1);
  ASSERT_EQ(confdata_ev->MAGIC, LEV_PMEMCACHED_DELETE);

  std::free(ev);
}

TEST(confdata_binlog_events_test, test_get_event) {
  const char key[] = "hello";
  auto *ev = my_malloc<lev_pmemcached_get>(std::strlen(key) - 1);
  ev->type = LEV_PMEMCACHED_GET;
  ev->hash = 123;
  ev->key_len = std::strlen(key);
  std::strcpy(ev->key, key);

  auto *confdata_ev = static_cast<lev_confdata_get *>(ev);
  ASSERT_EQ(confdata_ev->get_extra_bytes(), std::strlen(key) - 1);
  ASSERT_EQ(confdata_ev->MAGIC, LEV_PMEMCACHED_GET);

  std::free(ev);
}

TEST(confdata_binlog_events_test, test_touch_event) {
  const char key[] = "hello";
  auto *ev = my_malloc<lev_pmemcached_touch>(std::strlen(key) + 1);
  ev->type = LEV_PMEMCACHED_TOUCH;
  ev->delay = 123;
  ev->key_len = std::strlen(key);
  std::strcpy(ev->key, key);

  auto *confdata_ev = static_cast<lev_confdata_touch *>(ev);
  ASSERT_EQ(confdata_ev->get_extra_bytes(), std::strlen(key) + 1);
  ASSERT_EQ(confdata_ev->MAGIC, LEV_PMEMCACHED_TOUCH);

  std::free(ev);
}

TEST(confdata_binlog_events_test, test_incr_event) {
  const char key[] = "hello";
  auto *ev = my_malloc<lev_pmemcached_incr>(std::strlen(key) - 1);
  ev->type = LEV_PMEMCACHED_INCR;
  ev->arg = 100000;
  ev->key_len = std::strlen(key);
  std::strcpy(ev->key, key);

  auto *confdata_ev = static_cast<lev_confdata_incr *>(ev);
  ASSERT_EQ(confdata_ev->get_extra_bytes(), std::strlen(key) - 1);
  ASSERT_EQ(confdata_ev->MAGIC, LEV_PMEMCACHED_INCR);

  std::free(ev);
}

TEST(confdata_binlog_events_test, test_incr_tiny_event) {
  const char key[] = "hello";
  auto *ev = my_malloc<lev_pmemcached_incr_tiny>(std::strlen(key) - 1);
  ev->type = LEV_PMEMCACHED_INCR_TINY + 1;
  ev->key_len = std::strlen(key);
  std::strcpy(ev->key, key);

  auto *confdata_ev = static_cast<lev_confdata_incr_tiny_range *>(ev);
  ASSERT_EQ(confdata_ev->get_extra_bytes(), std::strlen(key) - 1);
  ASSERT_EQ(confdata_ev->MAGIC_BASE, LEV_PMEMCACHED_INCR_TINY);
  ASSERT_EQ(confdata_ev->MAGIC_MASK, 255);

  std::free(ev);
}

TEST(confdata_binlog_events_test, test_append_event) {
  const char key[] = "hello";
  const char data[] = " world!";
  auto *ev = my_malloc<lev_pmemcached_append>(std::strlen(key) + std::strlen(data));
  ev->type = LEV_PMEMCACHED_APPEND;
  ev->flags = 0;
  ev->delay = 1;
  ev->key_len = std::strlen(key);
  ev->data_len = std::strlen(data);
  std::strcpy(ev->data, key);
  // без \0
  std::memcpy(ev->data + std::strlen(key), data, std::strlen(data));

  auto *confdata_ev = static_cast<lev_confdata_append *>(ev);
  ASSERT_EQ(confdata_ev->get_extra_bytes(), std::strlen(key) + std::strlen(data));
  ASSERT_EQ(confdata_ev->MAGIC, LEV_PMEMCACHED_APPEND);

  std::free(ev);
}

template<int OPERATION>
void do_test_store_forever_event() {
  const char key[] = "hello";
  const char data[] = " world!";
  auto *ev = my_malloc<lev_pmemcached_store_forever>(std::strlen(key) + std::strlen(data) - 3);
  ev->type = LEV_PMEMCACHED_STORE_FOREVER + OPERATION;
  ev->key_len = std::strlen(key);
  ev->data_len = std::strlen(data);
  std::strcpy(ev->data, key);
  std::strcpy(ev->data + std::strlen(key), data);

  auto *confdata_ev = static_cast<lev_confdata_store_wrapper<lev_pmemcached_store_forever, OPERATION> *>(ev);
  ASSERT_EQ(confdata_ev->get_extra_bytes(), std::strlen(key) + std::strlen(data) - 3);
  ASSERT_EQ(confdata_ev->MAGIC, LEV_PMEMCACHED_STORE_FOREVER + OPERATION);
  ASSERT_EQ(confdata_ev->get_delay(), -1);
  ASSERT_EQ(confdata_ev->get_flags(), 0);
  ASSERT_TRUE(equals(confdata_ev->get_value_as_var(), string{data}));

  std::free(ev);
}

TEST(confdata_binlog_events_test, test_store_forever_event) {
  do_test_store_forever_event<pmct_add>();
  do_test_store_forever_event<pmct_set>();
  do_test_store_forever_event<pmct_replace>();
}

template<int OPERATION>
void do_test_store_event() {
  const char key[] = "hello";
  const char data[] = " world!";
  auto *ev = my_malloc<lev_pmemcached_store>(std::strlen(key) + std::strlen(data) - 3);
  ev->type = LEV_PMEMCACHED_STORE + OPERATION;
  ev->delay = 213;
  ev->flags = 1024;
  ev->key_len = std::strlen(key);
  ev->data_len = std::strlen(data);
  std::strcpy(ev->data, key);
  std::strcpy(ev->data + std::strlen(key), data);

  auto *confdata_ev = static_cast<lev_confdata_store_wrapper<lev_pmemcached_store, OPERATION> *>(ev);
  ASSERT_EQ(confdata_ev->get_extra_bytes(), std::strlen(key) + std::strlen(data) - 3);
  ASSERT_EQ(confdata_ev->MAGIC, LEV_PMEMCACHED_STORE + OPERATION);
  ASSERT_EQ(confdata_ev->get_delay(), 213);
  ASSERT_EQ(confdata_ev->get_flags(), 1024);
  ASSERT_TRUE(equals(confdata_ev->get_value_as_var(), string{data}));

  std::free(ev);
}

TEST(confdata_binlog_events_test, test_store_event) {
  do_test_store_event<pmct_add>();
  do_test_store_event<pmct_set>();
  do_test_store_event<pmct_replace>();
}

template<int OPERATION>
void do_test_index_entry_event() {
  const char key[] = "hello";
  const char data[] = " world!";
  auto *ev = my_malloc<index_entry>(std::strlen(key) + std::strlen(data) + 1);
  ev->delay = 213;
  ev->flags = 1024;
  ev->key_len = std::strlen(key);
  ev->data_len = std::strlen(data);
  std::strcpy(ev->data, key);
  std::strcpy(ev->data + std::strlen(key), data);

  auto *confdata_ev = static_cast<lev_confdata_store_wrapper<index_entry, OPERATION> *>(ev);
  ASSERT_EQ(confdata_ev->get_extra_bytes(), std::strlen(key) + std::strlen(data) + 1);
  ASSERT_EQ(confdata_ev->MAGIC, OPERATION);
  ASSERT_EQ(confdata_ev->get_delay(), 213);
  ASSERT_EQ(confdata_ev->get_flags(), 1024);
  ASSERT_TRUE(equals(confdata_ev->get_value_as_var(), string{data}));

  std::free(ev);
}

TEST(confdata_binlog_events_test, test_store_from_index_event) {
  do_test_index_entry_event<pmct_add>();
  do_test_index_entry_event<pmct_set>();
  do_test_index_entry_event<pmct_replace>();
}
