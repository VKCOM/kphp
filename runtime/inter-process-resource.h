// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <array>
#include <bitset>
#include <cstring>

#include "common/parallel/lock_accessible.h"
#include "common/wrappers/memory-utils.h"

#include "runtime/inter-process-mutex.h"
#include "runtime/php_assert.h"
#include "server/php-engine-vars.h"
#include "server/workers-control.h"

template<size_t RESOURCE_AMOUNT>
class InterProcessResourceControl {
public:
  static_assert(RESOURCE_AMOUNT != 0, "at least one resource is expected");

  InterProcessResourceControl() {
    for (auto &pids : acquired_pids_) {
      pids.fill(0);
    }
  }

  uint32_t acquire_active_resource_id() noexcept {
    const uint32_t resource_id = get_active_resource_id();
    acquired_pids_[resource_id][get_user_index()] = pid;
    return resource_id;
  }

  void release(uint32_t resource_id) noexcept {
    php_assert(resource_id < RESOURCE_AMOUNT);
    pid_t &stored_pid = acquired_pids_[resource_id][get_user_index()];
    php_assert(stored_pid == pid);
    stored_pid = 0;
  }

  void force_release_all_resources() noexcept {
    const auto user_index = get_user_index();
    for (size_t resource_id = 0; resource_id != RESOURCE_AMOUNT; ++resource_id) {
      acquired_pids_[resource_id][user_index] = 0;
    }
  }

  uint32_t get_active_resource_id() const noexcept {
    return active_resource_id_;
  }

  uint32_t get_next_inactive_resource_id() const noexcept {
    return (active_resource_id_ + 1) % RESOURCE_AMOUNT;
  }

  bool is_resource_unused(uint32_t resource_id) noexcept {
    php_assert(resource_id < RESOURCE_AMOUNT);
    constexpr static std::array<pid_t, WorkersControl::max_workers_count> zeros = {0};
    const auto worker_pid_it = acquired_pids_[resource_id].begin();
    return std::equal(worker_pid_it, worker_pid_it + vk::singleton<WorkersControl>::get().get_total_workers_count(), zeros.begin());
  }

  uint32_t switch_active_to_next() noexcept {
    const uint32_t prev_active = active_resource_id_;
    active_resource_id_ = get_next_inactive_resource_id();
    return prev_active;
  }

private:
  static size_t get_user_index() noexcept {
    php_assert(logname_id >= 0 && logname_id < WorkersControl::max_workers_count);
    return static_cast<size_t>(logname_id);
  }

  uint32_t active_resource_id_{0};
  std::array<std::array<pid_t, WorkersControl::max_workers_count>, RESOURCE_AMOUNT> acquired_pids_;
};

template<typename T, size_t RESOURCE_AMOUNT>
class InterProcessResourceManager {
public:
  InterProcessResourceManager() noexcept
    : initiate_process_pid_{pid} {}

  // called from master process once during initialization
  // all children inherit pointers and data from mem and control_block_
  template<typename... Args>
  void init(Args &&...args) noexcept {
    php_assert(is_initial_process());
    for (auto &resource : switchable_resource_) {
      resource.init(args...);
    }
    control_block_ = new (mmap_shared(sizeof(*control_block_))) std::decay_t<decltype(*control_block_)>{};
  }

  T *acquire_current_resource() noexcept {
    php_assert(control_block_);
    const uint32_t resource_id = (*control_block_)->acquire_active_resource_id();
    return &switchable_resource_[resource_id];
  }

  void release_resource(const T *data) noexcept {
    php_assert(control_block_);
    const auto it = std::find_if(switchable_resource_.begin(), switchable_resource_.end(), [data](const T &res) { return &res == data; });
    php_assert(it != switchable_resource_.end());
    const auto resource_id = static_cast<uint32_t>(it - switchable_resource_.begin());
    (*control_block_)->release(resource_id);
  }

  void force_release_all_resources() noexcept {
    (*control_block_)->force_release_all_resources();
  }

  // this function should be called only from master
  T &get_current_resource() noexcept {
    php_assert(is_initial_process());
    php_assert(control_block_);
    return switchable_resource_[(*control_block_)->get_active_resource_id()];
  }

  // this function should be called only from master
  bool is_next_resource_unused(uint32_t *inactive_resource_id_out = nullptr) noexcept {
    php_assert(is_initial_process());
    php_assert(control_block_);
    auto locked_block = control_block_->acquire_lock();
    const uint32_t inactive_resource_id = locked_block->get_next_inactive_resource_id();
    if (inactive_resource_id_out) {
      *inactive_resource_id_out = inactive_resource_id;
    }
    return locked_block->is_resource_unused(inactive_resource_id);
  }

  // this function should be called only from master
  template<typename... Args>
  bool try_switch_to_next_unused_resource(Args &&...args) noexcept {
    php_assert(is_initial_process());
    uint32_t inactive_resource_id = 0;
    if (is_next_resource_unused(&inactive_resource_id)) {
      switchable_resource_[inactive_resource_id].reset(std::forward<Args>(args)...);
      const uint32_t prev_active = (*control_block_)->switch_active_to_next();
      // previous become dirty
      dirty_inactive_resources_.set(prev_active);
      // new become not dirty
      dirty_inactive_resources_.reset(inactive_resource_id);
      return true;
    }
    return false;
  }

  // this function should be called only from master
  void clear_dirty_unused_resources_in_sequence() noexcept {
    php_assert(is_initial_process());
    if (dirty_inactive_resources_.none()) {
      return;
    }

    // resources are cleared strictly in the order they were marked as unused: starting with the oldest, etc.
    // if the oldest can't be cleared, the cleanup is stopped
    const uint32_t current_resource_id = (*control_block_)->get_active_resource_id();
    for (uint32_t resource_id = (current_resource_id + 1) % RESOURCE_AMOUNT; resource_id != current_resource_id && dirty_inactive_resources_.any();
         resource_id = (resource_id + 1) % RESOURCE_AMOUNT) {
      if (dirty_inactive_resources_.test(resource_id)) {
        if ((*control_block_)->is_resource_unused(resource_id)) {
          switchable_resource_[resource_id].clear();
          dirty_inactive_resources_.reset(resource_id);
        } else {
          break;
        }
      }
    }
  }

  bool is_initial_process() const noexcept {
    return initiate_process_pid_ == pid;
  }

  void destroy() noexcept {
    php_assert(control_block_);
    php_assert(is_initial_process());
    control_block_->~lock_accessible();
    munmap(control_block_, sizeof(*control_block_));
    control_block_ = nullptr;

    for (auto &resource : switchable_resource_) {
      resource.destroy();
    }
  }

  ~InterProcessResourceManager() noexcept {
    if (control_block_ && is_initial_process()) {
      destroy();
    }
  }

private:
  std::bitset<RESOURCE_AMOUNT> dirty_inactive_resources_;
  const pid_t initiate_process_pid_{0};
  std::array<T, RESOURCE_AMOUNT> switchable_resource_;
  vk::lock_accessible<InterProcessResourceControl<RESOURCE_AMOUNT>, inter_process_mutex> *control_block_{nullptr};
};
