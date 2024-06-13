//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-light/core/class_instance/refcountable_php_classes.h"
#include "runtime-light/core/kphp_core.h"

namespace job_worker_impl_ {

struct WorkerBase : virtual abstract_refcountable_php_interface {
  virtual const char *get_class() const noexcept = 0;
  virtual int32_t get_hash() const noexcept = 0;

  //  virtual void accept(InstanceReferencesCountingVisitor &) noexcept = 0;
  //  virtual void accept(InstanceDeepCopyVisitor &) noexcept = 0;
  //  virtual void accept(InstanceDeepDestroyVisitor &) noexcept = 0;
  //  virtual void accept([[maybe_unused]] ToArrayVisitor &v) noexcept {}
  //  virtual void accept([[maybe_unused]] CommonMemoryEstimateVisitor &v) noexcept {}

  virtual size_t virtual_builtin_sizeof() const noexcept = 0;
  virtual WorkerBase *virtual_builtin_clone() const noexcept = 0;

  ~WorkerBase() override = default;
};

} // namespace job_worker_impl_

enum class JobWorkerError : int16_t {
  store_response_incorrect_call_error = -3000,
  store_response_cant_send_error = -3003,
};

struct C$KphpJobWorkerRequest : job_worker_impl_::WorkerBase {
  C$KphpJobWorkerRequest *virtual_builtin_clone() const noexcept override = 0;
};

struct C$KphpJobWorkerResponse : job_worker_impl_::WorkerBase {
  C$KphpJobWorkerResponse *virtual_builtin_clone() const noexcept override = 0;
};

struct C$KphpJobWorkerResponseError : public refcountable_polymorphic_php_classes<C$KphpJobWorkerResponse> {
  string error;
  int64_t error_code;

  const char *get_class() const noexcept override;
  int32_t get_hash() const noexcept override;

  size_t virtual_builtin_sizeof() const noexcept override;
  C$KphpJobWorkerResponseError *virtual_builtin_clone() const noexcept override;
};

class_instance<C$KphpJobWorkerResponseError> f$KphpJobWorkerResponseError$$__construct(const class_instance<C$KphpJobWorkerResponseError> &v$this) noexcept;

string f$KphpJobWorkerResponseError$$getError(const class_instance<C$KphpJobWorkerResponseError> &v$this) noexcept;

int64_t f$KphpJobWorkerResponseError$$getErrorCode(const class_instance<C$KphpJobWorkerResponseError> &v$this) noexcept;
