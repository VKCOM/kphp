#include <cinttypes>
#include <gtest/gtest.h>

#include "common/crc32.h"
#include "server/cluster-name.h"

TEST(cluster_name_test, test_usage) {
  ASSERT_EQ(vk::singleton<ClusterName>::get().set_cluster_name("kphp_engine"), nullptr);

  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_cluster_name(), "kphp_engine");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_socket_name(), "kphp_master_fd_for_graceful_restart-kphp_engine");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_shmem_name(), "/kphp_engine_kphp_shm");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_statsd_prefix(), "kphp_stats.kphp_engine");
}

TEST(cluster_name_test, test_long_name) {
  ASSERT_STREQ(vk::singleton<ClusterName>::get().set_cluster_name("default"), nullptr);
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_cluster_name(), "default");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_socket_name(), "kphp_master_fd_for_graceful_restart-default");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_shmem_name(), "/default_kphp_shm");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_statsd_prefix(), "kphp_stats.default");

  const std::string long_cluster_name = "kphp" + std::string(250, '_') + "_engine";
  ASSERT_STREQ(vk::singleton<ClusterName>::get().set_cluster_name(long_cluster_name.c_str()), "too long cluster name");

  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_cluster_name(), "default");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_socket_name(), "kphp_master_fd_for_graceful_restart-default");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_shmem_name(), "/default_kphp_shm");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_statsd_prefix(), "kphp_stats.default");

  const std::string not_very_long_cluster_name = "kphp" + std::string(128, '-') + "_engine";
  ASSERT_EQ(vk::singleton<ClusterName>::get().set_cluster_name(not_very_long_cluster_name.c_str()), nullptr);
  uint64_t crc = compute_crc64(not_very_long_cluster_name.c_str(), not_very_long_cluster_name.length());
  char hex_crc_str[sizeof(crc) * 2 + 1];
  std::sprintf(hex_crc_str, "%16" PRIx64, crc);
  std::string expected_socket_name = "kphp_master_fd_for_graceful_restart-" + std::string(hex_crc_str);
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_cluster_name(), not_very_long_cluster_name.c_str());
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_socket_name(), expected_socket_name.c_str());
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_shmem_name(), ("/" + not_very_long_cluster_name + "_kphp_shm").c_str());
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_statsd_prefix(), ("kphp_stats." + not_very_long_cluster_name).c_str());
}

TEST(cluster_name_test, test_empty_name) {
  ASSERT_STREQ(vk::singleton<ClusterName>::get().set_cluster_name("default"), nullptr);
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_cluster_name(), "default");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_socket_name(), "kphp_master_fd_for_graceful_restart-default");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_shmem_name(), "/default_kphp_shm");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_statsd_prefix(), "kphp_stats.default");

  ASSERT_STREQ(vk::singleton<ClusterName>::get().set_cluster_name(""), "empty cluster name");

  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_cluster_name(), "default");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_socket_name(), "kphp_master_fd_for_graceful_restart-default");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_shmem_name(), "/default_kphp_shm");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_statsd_prefix(), "kphp_stats.default");
}

TEST(cluster_name_test, test_special_chars) {
  ASSERT_STREQ(vk::singleton<ClusterName>::get().set_cluster_name("a-b1c*d/e+f~g<h?i"),
               "Incorrect symbol in cluster name. Allowed symbols are: alpha-numerics, '-', '_'");
  ASSERT_EQ(vk::singleton<ClusterName>::get().set_cluster_name("aB-12_0xD"), nullptr);
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_cluster_name(), "aB-12_0xD");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_socket_name(), "kphp_master_fd_for_graceful_restart-aB-12_0xD");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_shmem_name(), "/aB-12_0xD_kphp_shm");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_statsd_prefix(), "kphp_stats.aB-12_0xD");
}
