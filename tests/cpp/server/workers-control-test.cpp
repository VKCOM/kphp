#include <gtest/gtest.h>

#include "server/workers-control.h"

#define ASSERT_WORKERS(t, running, dying, all_alive)                                                                                                           \
  {                                                                                                                                                            \
    ASSERT_EQ(control.get_all_alive(), all_alive);                                                                                                             \
    ASSERT_EQ(control.get_running_count(t), running);                                                                                                          \
    ASSERT_EQ(control.get_dying_count(t), dying);                                                                                                              \
    ASSERT_EQ(control.get_alive_count(t), (running) + (dying));                                                                                                \
  }

TEST(workers_control_test, test_small_workers_count) {
  auto &control = vk::singleton<WorkersControl>::get();
  control.set_total_workers_count(2);
  ASSERT_EQ(control.get_total_workers_count(), 2);

  control.set_ratio(WorkerType::job_worker, 0.9);
  ASSERT_FALSE(control.init());
}

TEST(workers_control_test, test_workers_count) {
  auto &control = vk::singleton<WorkersControl>::get();
  control.set_total_workers_count(356);
  ASSERT_EQ(control.get_total_workers_count(), 356);

  control.set_ratio(WorkerType::job_worker, 0.3);
  ASSERT_TRUE(control.init());

  ASSERT_EQ(control.get_count(WorkerType::job_worker), 107);
  ASSERT_EQ(control.get_count(WorkerType::general_worker), 249);

  for (WorkerType type : {WorkerType::job_worker, WorkerType::general_worker}) {
    ASSERT_WORKERS(type, 0, 0, 0);
  }

  ASSERT_EQ(control.on_worker_creating(WorkerType::general_worker), 248);
  ASSERT_EQ(control.on_worker_creating(WorkerType::general_worker), 247);
  ASSERT_EQ(control.on_worker_creating(WorkerType::job_worker), 355);
  ASSERT_EQ(control.on_worker_creating(WorkerType::job_worker), 354);

  for (WorkerType type : {WorkerType::job_worker, WorkerType::general_worker}) {
    ASSERT_WORKERS(type, 2, 0, 4);
  }

  for (int i = 246; i >= 0; --i) {
    ASSERT_EQ(control.on_worker_creating(WorkerType::general_worker), i);
  }
  ASSERT_WORKERS(WorkerType::general_worker, 249, 0, 251);

  for (int i = 353; i >= 249; --i) {
    ASSERT_EQ(control.on_worker_creating(WorkerType::job_worker), i);
  }
  ASSERT_WORKERS(WorkerType::job_worker, 107, 0, 356);

  control.on_worker_terminating(WorkerType::general_worker);
  control.on_worker_terminating(WorkerType::general_worker);
  control.on_worker_terminating(WorkerType::general_worker);
  ASSERT_WORKERS(WorkerType::general_worker, 246, 3, 356);
  ASSERT_WORKERS(WorkerType::job_worker, 107, 0, 356);

  control.on_worker_terminating(WorkerType::job_worker);
  control.on_worker_terminating(WorkerType::job_worker);
  ASSERT_WORKERS(WorkerType::general_worker, 246, 3, 356);
  ASSERT_WORKERS(WorkerType::job_worker, 105, 2, 356);

  control.on_worker_removing(WorkerType::general_worker, true, 100);
  control.on_worker_removing(WorkerType::general_worker, false, 105);
  ASSERT_WORKERS(WorkerType::general_worker, 245, 2, 354);
  ASSERT_WORKERS(WorkerType::job_worker, 105, 2, 354);

  control.on_worker_removing(WorkerType::job_worker, true, 300);
  control.on_worker_removing(WorkerType::job_worker, false, 350);
  ASSERT_WORKERS(WorkerType::general_worker, 245, 2, 352);
  ASSERT_WORKERS(WorkerType::job_worker, 104, 1, 352);

  ASSERT_EQ(control.on_worker_creating(WorkerType::general_worker), 105);
  ASSERT_EQ(control.on_worker_creating(WorkerType::job_worker), 350);
  ASSERT_EQ(control.on_worker_creating(WorkerType::general_worker), 100);
  ASSERT_EQ(control.on_worker_creating(WorkerType::job_worker), 300);
  ASSERT_WORKERS(WorkerType::general_worker, 247, 2, 356);
  ASSERT_WORKERS(WorkerType::job_worker, 106, 1, 356);
}
