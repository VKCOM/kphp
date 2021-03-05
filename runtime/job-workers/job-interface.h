// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/refcountable_php_classes.h"

class InstanceDeepCopyVisitor;

class InstanceDeepDestroyVisitor;

class InstanceToArrayVisitor;

class InstanceMemoryEstimateVisitor;

namespace job_workers {

struct SendingInstanceBase : abstract_refcountable_php_interface {
  virtual const char *get_class() const noexcept = 0;
  virtual int get_hash() const noexcept = 0;

  virtual void accept(InstanceDeepCopyVisitor &) noexcept = 0;
  virtual void accept(InstanceDeepDestroyVisitor &) noexcept = 0;

  virtual void accept(InstanceToArrayVisitor &) noexcept {}

  virtual void accept(InstanceMemoryEstimateVisitor &) noexcept {}

  virtual size_t virtual_builtin_sizeof() const noexcept = 0;
  virtual SendingInstanceBase *virtual_builtin_clone() const noexcept = 0;

  virtual ~SendingInstanceBase() = default;
};

struct SharedMemorySlice;

} // job_workers

struct C$KphpJobWorkerRequest : job_workers::SendingInstanceBase {
  C$KphpJobWorkerRequest *virtual_builtin_clone() const noexcept override = 0;
};

struct C$KphpJobWorkerReply : job_workers::SendingInstanceBase {
  C$KphpJobWorkerReply *virtual_builtin_clone() const noexcept override = 0;
};

bool f$is_job_workers_enabled() noexcept;

void global_init_job_workers_lib() noexcept;

void process_job_worker_answer_event(int ready_job_id, job_workers::SharedMemorySlice *job_result_script_memory_ptr) noexcept;
