// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>

#include "common/smart_ptrs/singleton.h"
#include "common/timer.h"
#include "server/php-master-restart.h"
#include "server/workers-control.h"

class WarmUpContext : public vk::singleton<WarmUpContext> {
public:
  uint32_t get_final_new_instance_cache_size() const {
    return final_new_instance_cache_size_;
  }

  uint32_t get_final_old_instance_cache_size() const {
    return final_old_instance_cache_size_;
  }

  void update_final_instance_cache_sizes() {
    if (!final_instance_cache_sizes_saved_) {
      final_new_instance_cache_size_ = me->instance_cache_elements_cached;
      final_old_instance_cache_size_ = other->instance_cache_elements_cached;
      final_instance_cache_sizes_saved_ = true;
    }
  }

  void try_start_warmup() {
    if (control_.get_running_count(WorkerType::general_worker) > 0 && !timer_.is_started()) {
      timer_.start();
    }
  }

  void reset() {
    timer_.reset();
    final_new_instance_cache_size_ = 0;
    final_old_instance_cache_size_ = 0;
    final_instance_cache_sizes_saved_ = false;
  }

  bool need_more_workers_for_warmup() const {
    return control_.get_running_count(WorkerType::general_worker) < workers_part_for_warm_up_ * control_.get_count(WorkerType::general_worker);
  }

  bool is_instance_cache_hot_enough() const {
    return me->instance_cache_elements_cached >= target_instance_cache_elements_part_ * other->instance_cache_elements_cached;
  }

  bool warmup_timeout_expired() const {
    return timer_.time() > warm_up_max_time_;
  }

  void set_workers_part_for_warm_up(double workers_part_for_warm_up) {
    this->workers_part_for_warm_up_ = workers_part_for_warm_up;
  }

  void set_target_instance_cache_elements_part(double target_instance_cache_elements_part) {
    this->target_instance_cache_elements_part_ = target_instance_cache_elements_part;
  }

  void set_warm_up_max_time(const std::chrono::duration<double> &warm_up_max_time) {
    this->warm_up_max_time_ = warm_up_max_time;
  }

private:
  double workers_part_for_warm_up_{1};
  double target_instance_cache_elements_part_{0};
  std::chrono::duration<double> warm_up_max_time_{5.0};

  vk::SteadyTimer<std::chrono::milliseconds> timer_{};

  uint32_t final_new_instance_cache_size_{0};
  uint32_t final_old_instance_cache_size_{0};
  bool final_instance_cache_sizes_saved_{false};

  const WorkersControl &control_;

  WarmUpContext() noexcept
    : control_(vk::singleton<WorkersControl>::get()) {}

  friend vk::singleton<WarmUpContext>;
};
