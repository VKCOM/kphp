// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/algorithms/hashes.h"
#include "common/wrappers/string_view.h"

#include "runtime/instance-copy-processor.h"
#include "runtime/kphp_core.h"
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

struct JobSharedMessage;

} // job_workers

struct C$KphpJobWorkerRequest : job_workers::SendingInstanceBase {
  C$KphpJobWorkerRequest *virtual_builtin_clone() const noexcept override = 0;
};

struct C$KphpJobWorkerResponse : job_workers::SendingInstanceBase {
  C$KphpJobWorkerResponse *virtual_builtin_clone() const noexcept override = 0;
};

struct C$KphpJobWorkerResponseError: public refcountable_polymorphic_php_classes<C$KphpJobWorkerResponse> {
  string error;
  int64_t error_code;

  const char *get_class() const  noexcept {
    return R"(KphpJobWorkerResponseError)";
  }

  int get_hash() const noexcept {
    return static_cast<int32_t>(vk::std_hash(vk::string_view(C$KphpJobWorkerResponseError::get_class())));
  }

  template<class Visitor>
  void generic_accept(Visitor &&visitor) noexcept {
    visitor("error", error);
    visitor("error_code", error_code);
  }

  void accept(InstanceDeepCopyVisitor &visitor) noexcept {
    return generic_accept(visitor);
  }

  void accept(InstanceDeepDestroyVisitor &visitor) noexcept {
    return generic_accept(visitor);
  }

  size_t virtual_builtin_sizeof() const  noexcept {
    return sizeof(*this);
  }

  C$KphpJobWorkerResponseError* virtual_builtin_clone() const  noexcept {
    return new C$KphpJobWorkerResponseError{*this};
  }
};

class_instance<C$KphpJobWorkerResponseError> f$KphpJobWorkerResponseError$$__construct(class_instance<C$KphpJobWorkerResponseError> const &v$this) noexcept;
string f$KphpJobWorkerResponseError$$getError(class_instance<C$KphpJobWorkerResponseError> const &v$this) noexcept;
int64_t f$KphpJobWorkerResponseError$$getErrorCode(class_instance<C$KphpJobWorkerResponseError> const &v$this) noexcept;

bool f$is_kphp_job_workers_enabled() noexcept;

void global_init_job_workers_lib() noexcept;

void process_job_worker_answer_event(job_workers::JobSharedMessage *job_result) noexcept;
