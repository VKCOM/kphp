#include <cinttypes>
#include <gtest/gtest.h>

#include "common/crc32.h"
#include "server/master-name.h"

TEST(master_name_test, test_usage) {
  ASSERT_EQ(vk::singleton<MasterName>::get().set_master_name("kphp_engine", false), nullptr);

  ASSERT_STREQ(vk::singleton<MasterName>::get().get_master_name(), "kphp_engine");
  ASSERT_STREQ(vk::singleton<MasterName>::get().get_socket_name(), "kphp_engine_kphp_fd_transfer");
  ASSERT_STREQ(vk::singleton<MasterName>::get().get_shmem_name(), "/kphp_engine_kphp_shm");
}

TEST(master_name_test, test_long_name) {
  ASSERT_STREQ(vk::singleton<MasterName>::get().set_master_name("default", false), nullptr);

  ASSERT_STREQ(vk::singleton<MasterName>::get().get_master_name(), "default");
  ASSERT_STREQ(vk::singleton<MasterName>::get().get_socket_name(), "default_kphp_fd_transfer");
  ASSERT_STREQ(vk::singleton<MasterName>::get().get_shmem_name(), "/default_kphp_shm");

  const std::string long_cluster_name = "kphp" + std::string(250, '_') + "_engine";
  ASSERT_STREQ(vk::singleton<MasterName>::get().set_master_name(long_cluster_name.c_str(), false), "too long master name");

  ASSERT_STREQ(vk::singleton<MasterName>::get().get_master_name(), "default");
  ASSERT_STREQ(vk::singleton<MasterName>::get().get_socket_name(), "default_kphp_fd_transfer");
  ASSERT_STREQ(vk::singleton<MasterName>::get().get_shmem_name(), "/default_kphp_shm");

  const std::string not_very_long_cluster_name = "kphp" + std::string(128, '-') + "_engine";
  ASSERT_EQ(vk::singleton<MasterName>::get().set_master_name(not_very_long_cluster_name.c_str(), false), nullptr);

  uint64_t crc = compute_crc64(not_very_long_cluster_name.c_str(), not_very_long_cluster_name.length());
  size_t hex_crc_str_size = sizeof(crc) * 2 + 1;
  char hex_crc_str[hex_crc_str_size];
  std::snprintf(hex_crc_str, hex_crc_str_size, "%16" PRIx64, crc);
  std::string expected_socket_name = std::string(hex_crc_str) + "_kphp_fd_transfer";
  ASSERT_STREQ(vk::singleton<MasterName>::get().get_master_name(), not_very_long_cluster_name.c_str());
  ASSERT_STREQ(vk::singleton<MasterName>::get().get_socket_name(), expected_socket_name.c_str());
  ASSERT_STREQ(vk::singleton<MasterName>::get().get_shmem_name(), ("/" + not_very_long_cluster_name + "_kphp_shm").c_str());
}

TEST(master_name_test, test_empty_name) {
  ASSERT_STREQ(vk::singleton<MasterName>::get().set_master_name("default", false), nullptr);

  ASSERT_STREQ(vk::singleton<MasterName>::get().get_master_name(), "default");
  ASSERT_STREQ(vk::singleton<MasterName>::get().get_socket_name(), "default_kphp_fd_transfer");
  ASSERT_STREQ(vk::singleton<MasterName>::get().get_shmem_name(), "/default_kphp_shm");

  ASSERT_STREQ(vk::singleton<MasterName>::get().set_master_name("", false), "empty master name");

  ASSERT_STREQ(vk::singleton<MasterName>::get().get_master_name(), "default");
  ASSERT_STREQ(vk::singleton<MasterName>::get().get_socket_name(), "default_kphp_fd_transfer");
  ASSERT_STREQ(vk::singleton<MasterName>::get().get_shmem_name(), "/default_kphp_shm");
}

TEST(master_name_test, test_special_chars) {
  ASSERT_STREQ(vk::singleton<MasterName>::get().set_master_name("a-b1c*d/e+f~g<h?i", false),
               "Incorrect symbol in master name. Allowed symbols are: alpha-numerics, '-', '_'");

  ASSERT_EQ(vk::singleton<MasterName>::get().set_master_name("aB-12_0xD", false), nullptr);

  ASSERT_STREQ(vk::singleton<MasterName>::get().get_master_name(), "aB-12_0xD");
  ASSERT_STREQ(vk::singleton<MasterName>::get().get_socket_name(), "aB-12_0xD_kphp_fd_transfer");
  ASSERT_STREQ(vk::singleton<MasterName>::get().get_shmem_name(), "/aB-12_0xD_kphp_shm");
}
