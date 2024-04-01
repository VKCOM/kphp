// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cinttypes>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"

enum class WorkerType {
  general_worker = 0, // for http, task, rpc stuff
  job_worker = 1,     // for parallel job stuff
  types_count
};

class WorkersControl : vk::not_copyable {
public:
  static constexpr uint16_t max_workers_count{1000};

  bool init() noexcept;

  void set_total_workers_count(uint16_t workers_count) noexcept {
    total_workers_count_ = workers_count;
  }

  uint16_t get_total_workers_count() const noexcept {
    return total_workers_count_;
  }

  uint16_t get_count(WorkerType worker_type) const noexcept {
    return meta_[static_cast<size_t>(worker_type)].count;
  }

  uint16_t get_running_count(WorkerType worker_type) const noexcept {
    return meta_[static_cast<size_t>(worker_type)].running;
  }

  uint16_t get_dying_count(WorkerType worker_type) const noexcept {
    return meta_[static_cast<size_t>(worker_type)].dying;
  }

  uint16_t get_alive_count(WorkerType worker_type) const noexcept {
    const auto &meta = meta_[static_cast<size_t>(worker_type)];
    return meta.running + meta.dying;
  }

  uint16_t get_all_alive() const noexcept {
    uint16_t result = 0;
    for (size_t type = 0; type != meta_.size(); ++type) {
      result += get_alive_count(static_cast<WorkerType>(type));
    }
    return result;
  }

  void set_ratio(WorkerType worker_type, double ratio) noexcept {
    meta_[static_cast<size_t>(worker_type)].ratio = ratio;
  }

  uint16_t on_worker_creating(WorkerType worker_type) noexcept;
  void on_worker_terminating(WorkerType worker_type) noexcept;
  void on_worker_removing(WorkerType worker_type, bool dying, uint16_t worker_unique_id) noexcept;

private:
  WorkersControl() = default;

  friend class vk::singleton<WorkersControl>;

  struct WorkerGroupMetadata : vk::not_copyable {
    double ratio{0};

    uint16_t count{0};
    uint16_t running{0};

    uint16_t dying{0};

    std::array<uint16_t, max_workers_count> unique_ids_{};
  };
  uint16_t total_workers_count_{0};
  std::array<WorkerGroupMetadata, static_cast<size_t>(WorkerType::types_count)> meta_;
};
