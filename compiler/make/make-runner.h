// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <map>
#include <queue>

#include "common/mixin/not_copyable.h"

#include "compiler/make/target.h"

class MakeRunner : private vk::not_copyable {
  class compare_by_priority {
  public:
    bool operator()(Target* a, Target* b) const {
      return a->priority < b->priority;
    }
  };

private:
  int targets_waiting = 0;
  int targets_left = 0;
  std::vector<Target*> all_targets;
  FILE* stats_file_{nullptr};

  std::priority_queue<Target*, std::vector<Target*>, compare_by_priority> pending_jobs;
  std::map<int, Target*> jobs;

  bool fail_flag = false;
  static int signal_flag;
  static void sigint_handler(int sig);

  bool start_job(Target* target) __attribute__((warn_unused_result));
  bool finish_job(int pid, int return_code, int by_signal) __attribute__((warn_unused_result));
  void on_fail();

  void run_target(Target* target);
  void ready_target(Target* target);
  void one_dep_ready_target(Target* target);
  void wait_target(Target* target);
  void require_target(Target* target);

public:
  void register_target(Target* target, std::vector<Target*>&& deps);
  bool make_targets(const std::vector<Target*>& target, const std::string& build_message, std::size_t jobs_count = 32);
  explicit MakeRunner(FILE* stats_file) noexcept;
  ~MakeRunner();
};
