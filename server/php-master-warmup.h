// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>

#include "common/smart_ptrs/singleton.h"
#include "common/timer.h"
#include "server/php-master-restart.h"

class WarmUpContext : public vk::singleton<WarmUpContext> {
public:
  void try_start_warmup() {
    if (me_running_workers_n > 0 && !timer_.is_started()) {
      timer_.start();
    }
  }

  void reset() {
    timer_.reset();
  }

  bool need_more_workers_for_warmup() const {
    return me_running_workers_n < workers_part_for_warm_up_ * workers_n;
  }

  bool is_instance_cache_hot_enough() const {
    return me->instance_cache_elements_stored >= target_instance_cache_elements_part_ * other->instance_cache_elements_stored;
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

  WarmUpContext() = default;

  friend vk::singleton<WarmUpContext>;
};
