#include <gtest/gtest.h>

#include "runtime/inter-process-resource.h"

void set_pid_and_user_id(pid_t p) {
  // прибиваем гвоздями, используется в InterProcessResourceControl для определения кол-ва воркеров
  vk::singleton<WorkersControl>::get().set_total_workers_count(10);
  // pid используется как уникальный идентификатор в InterProcessResourceControl
  pid = p;
  // logname_id используется как id процесса от 0 до workers_count
  logname_id = p;
}

struct ResourceStub {
  void init(int v) {
    value = v;
  }
  void reset(int v) {
    value = v;
  }
  void clear() {
    value = 0;
  }
  void destroy() {
    value = -1;
  }

  int value{0};
};

TEST(inter_process_resource_control_test, test_acquire_release) {
  InterProcessResourceControl<5> resource_control;

  set_pid_and_user_id(1);
  ASSERT_EQ(resource_control.get_active_resource_id(), 0);

  ASSERT_EQ(resource_control.acquire_active_resource_id(), 0);
  ASSERT_FALSE(resource_control.is_resource_unused(0));

  resource_control.release(0);
  ASSERT_TRUE(resource_control.is_resource_unused(0));

  set_pid_and_user_id(1);
  ASSERT_EQ(resource_control.acquire_active_resource_id(), 0);
  ASSERT_FALSE(resource_control.is_resource_unused(0));

  set_pid_and_user_id(2);
  ASSERT_EQ(resource_control.acquire_active_resource_id(), 0);
  ASSERT_FALSE(resource_control.is_resource_unused(0));

  set_pid_and_user_id(1);
  resource_control.release(0);
  ASSERT_FALSE(resource_control.is_resource_unused(0));

  set_pid_and_user_id(2);
  resource_control.release(0);
  ASSERT_TRUE(resource_control.is_resource_unused(0));
}

TEST(inter_process_resource_control_test, test_switch_active_to_next) {
  InterProcessResourceControl<7> resource_control;

  set_pid_and_user_id(1);
  for (uint32_t i = 0; i < 10; ++i) {
    ASSERT_EQ(resource_control.get_next_inactive_resource_id(), (i + 1) % 7);
    ASSERT_EQ(resource_control.get_active_resource_id(), i % 7);

    ASSERT_EQ(resource_control.switch_active_to_next(), i % 7);
    ASSERT_EQ(resource_control.get_active_resource_id(), (i + 1) % 7);
    ASSERT_EQ(resource_control.get_next_inactive_resource_id(), (i + 2) % 7);
  }
}

TEST(inter_process_resource_control_test, test_force_release_all_resources) {
  InterProcessResourceControl<5> resource_control;

  set_pid_and_user_id(1);
  for (uint32_t i = 0; i < 4; ++i) {
    ASSERT_EQ(resource_control.acquire_active_resource_id(), i);
    ASSERT_EQ(resource_control.switch_active_to_next(), i);
    ASSERT_FALSE(resource_control.is_resource_unused(i));
  }

  resource_control.force_release_all_resources();
  for (uint32_t i = 0; i < 4; ++i) {
    ASSERT_TRUE(resource_control.is_resource_unused(i));
  }
}

TEST(inter_process_resource_manager_test, test_constructor_destructor) {
  set_pid_and_user_id(1);
  InterProcessResourceManager<ResourceStub, 3> resource_manager;
}

TEST(inter_process_resource_manager_test, test_is_initial_process) {
  set_pid_and_user_id(1);
  InterProcessResourceManager<ResourceStub, 3> resource_manager;
  ASSERT_TRUE(resource_manager.is_initial_process());

  set_pid_and_user_id(2);
  ASSERT_FALSE(resource_manager.is_initial_process());

  set_pid_and_user_id(1);
  ASSERT_TRUE(resource_manager.is_initial_process());
}

TEST(inter_process_resource_manager_test, test_init_and_destroy) {
  set_pid_and_user_id(1);

  InterProcessResourceManager<ResourceStub, 3> resource_manager;
  resource_manager.init(100500);

  const int &resource_value = resource_manager.get_current_resource().value;
  ASSERT_EQ(resource_value, 100500);

  resource_manager.destroy();
  ASSERT_EQ(resource_value, -1);
}

TEST(inter_process_resource_manager_test, test_acqure_release_resource) {
  set_pid_and_user_id(1);

  InterProcessResourceManager<ResourceStub, 3> resource_manager;
  resource_manager.init(100500);

  auto *resource1 = resource_manager.acquire_current_resource();
  ASSERT_TRUE(resource1);
  ASSERT_EQ(resource1->value, 100500);

  set_pid_and_user_id(2);
  auto *resource2 = resource_manager.acquire_current_resource();
  ASSERT_TRUE(resource2);
  ASSERT_EQ(resource2->value, 100500);
  ASSERT_EQ(resource1, resource2);

  set_pid_and_user_id(1);
  ASSERT_EQ(resource1, &resource_manager.get_current_resource());
  resource_manager.release_resource(resource1);

  set_pid_and_user_id(2);
  resource_manager.release_resource(resource2);
}

TEST(inter_process_resource_manager_test, test_is_next_resource_unused) {
  set_pid_and_user_id(1);

  InterProcessResourceManager<ResourceStub, 3> resource_manager;
  resource_manager.init(100500);

  for (uint32_t i = 0; i < 10; ++i) {
    uint32_t inactive_resource_id = std::numeric_limits<uint32_t>::max();
    ASSERT_TRUE(resource_manager.is_next_resource_unused());
    ASSERT_TRUE(resource_manager.is_next_resource_unused(&inactive_resource_id));
    ASSERT_EQ(inactive_resource_id, (i + 1) % 3);
    ASSERT_TRUE(resource_manager.try_switch_to_next_unused_resource(100 + i));
    ASSERT_EQ(resource_manager.get_current_resource().value, 100 + i);
  }

  for (uint32_t i = 0; i < 3; ++i) {
    ASSERT_TRUE(resource_manager.try_switch_to_next_unused_resource(200 + i));
    ASSERT_TRUE(resource_manager.acquire_current_resource());
  }

  ASSERT_FALSE(resource_manager.is_next_resource_unused());
  ASSERT_FALSE(resource_manager.try_switch_to_next_unused_resource(0));
}

TEST(inter_process_resource_manager_test, acquire_release_clear_scenario) {
  set_pid_and_user_id(1);

  InterProcessResourceManager<ResourceStub, 3> resource_manager;
  resource_manager.init(111);

  set_pid_and_user_id(2);
  auto *r2 = resource_manager.acquire_current_resource();
  ASSERT_TRUE(r2);
  ASSERT_EQ(r2->value, 111);

  set_pid_and_user_id(1);
  ASSERT_TRUE(resource_manager.try_switch_to_next_unused_resource(222));

  set_pid_and_user_id(3);
  auto *r3 = resource_manager.acquire_current_resource();
  ASSERT_TRUE(r3);
  ASSERT_EQ(r3->value, 222);

  set_pid_and_user_id(1);
  resource_manager.clear_dirty_unused_resources_in_sequence();
  ASSERT_EQ(r2->value, 111);

  set_pid_and_user_id(2);
  resource_manager.release_resource(r2);

  set_pid_and_user_id(1);
  resource_manager.clear_dirty_unused_resources_in_sequence();
  ASSERT_EQ(r2->value, 0);
}

TEST(inter_process_resource_manager_test, acquire_force_release_clear_scenario) {
  set_pid_and_user_id(1);

  InterProcessResourceManager<ResourceStub, 3> resource_manager;
  resource_manager.init(111);

  set_pid_and_user_id(2);
  auto *r21 = resource_manager.acquire_current_resource();
  ASSERT_TRUE(r21);
  ASSERT_EQ(r21->value, 111);

  set_pid_and_user_id(1);
  ASSERT_TRUE(resource_manager.try_switch_to_next_unused_resource(222));

  set_pid_and_user_id(2);
  auto *r22 = resource_manager.acquire_current_resource();
  ASSERT_TRUE(r22);
  ASSERT_EQ(r22->value, 222);

  set_pid_and_user_id(1);
  ASSERT_TRUE(resource_manager.try_switch_to_next_unused_resource(333));
  resource_manager.clear_dirty_unused_resources_in_sequence();
  ASSERT_EQ(r21->value, 111);
  ASSERT_EQ(r22->value, 222);

  set_pid_and_user_id(2);
  resource_manager.force_release_all_resources();

  set_pid_and_user_id(1);
  resource_manager.clear_dirty_unused_resources_in_sequence();
  ASSERT_EQ(r21->value, 0);
  ASSERT_EQ(r22->value, 0);
}
