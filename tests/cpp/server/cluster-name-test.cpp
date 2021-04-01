#include <gtest/gtest.h>

#include "server/cluster-name.h"

TEST(cluster_name_test, test_usage) {
  ASSERT_EQ(vk::singleton<ClusterName>::get().set_cluster_name("kphp_engine"), nullptr);

  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_cluster_name(), "kphp_engine");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_socket_name(), "kphp_engine_kphp_fd_transfer");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_shmem_name(), "/kphp_engine_kphp_shm");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_statsd_prefix(), "kphp_stats.kphp_engine");
}

TEST(cluster_name_test, test_long_name) {
  ASSERT_STREQ(vk::singleton<ClusterName>::get().set_cluster_name("default"), nullptr);
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_cluster_name(), "default");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_socket_name(), "default_kphp_fd_transfer");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_shmem_name(), "/default_kphp_shm");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_statsd_prefix(), "kphp_stats.default");

  const std::string long_cluster_name = "kphp" + std::string(250, '_') + "_engine";
  ASSERT_STREQ(vk::singleton<ClusterName>::get().set_cluster_name(long_cluster_name.c_str()), "too long cluster name");

  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_cluster_name(), "default");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_socket_name(), "default_kphp_fd_transfer");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_shmem_name(), "/default_kphp_shm");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_statsd_prefix(), "kphp_stats.default");
}

TEST(cluster_name_test, test_empty_name) {
  ASSERT_STREQ(vk::singleton<ClusterName>::get().set_cluster_name("default"), nullptr);
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_cluster_name(), "default");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_socket_name(), "default_kphp_fd_transfer");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_shmem_name(), "/default_kphp_shm");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_statsd_prefix(), "kphp_stats.default");

  ASSERT_STREQ(vk::singleton<ClusterName>::get().set_cluster_name(""), "empty cluster name");

  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_cluster_name(), "default");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_socket_name(), "default_kphp_fd_transfer");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_shmem_name(), "/default_kphp_shm");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_statsd_prefix(), "kphp_stats.default");
}

TEST(cluster_name_test, test_special_chars) {
  ASSERT_EQ(vk::singleton<ClusterName>::get().set_cluster_name("a-b1c*d/e+f~g<h?i"), nullptr);

  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_cluster_name(), "a-b1c*d/e+f~g<h?i");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_socket_name(), "a_b_c_d_e_f_g_h_i_kphp_fd_transfer");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_shmem_name(), "/a_b_c_d_e_f_g_h_i_kphp_shm");
  ASSERT_STREQ(vk::singleton<ClusterName>::get().get_statsd_prefix(), "kphp_stats.a_b_c_d_e_f_g_h_i");
}
