// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

#include "common/algorithms/hashes.h"
#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/core/runtime-core.h"

inline constexpr int64_t JOB_WORKER_VALID_JOB_ID_RANGE_START = 0;
inline constexpr int64_t JOB_WORKER_INVALID_JOB_ID = -1;

namespace job_worker_impl_ {

class ToArrayVisitor;

struct SendableBase : virtual abstract_refcountable_php_interface {

  virtual void accept(ToArrayVisitor & /*unused*/) noexcept {}

  virtual const char *get_class() const noexcept = 0;
  virtual int32_t get_hash() const noexcept = 0;
  virtual size_t virtual_builtin_sizeof() const noexcept = 0;
  virtual SendableBase *virtual_builtin_clone() const noexcept = 0;

  ~SendableBase() override = default;
};

} // namespace job_worker_impl_

enum class JobWorkerError : int16_t {
  store_response_incorrect_call_error = -3000,
  store_response_cant_send_error = -3003,
};

// === KphpJobWorkerSharedMemoryPiece =============================================================

struct C$KphpJobWorkerSharedMemoryPiece : public job_worker_impl_::SendableBase {
  C$KphpJobWorkerSharedMemoryPiece *virtual_builtin_clone() const noexcept override = 0;
};

// === KphpJobWorkerRequest =======================================================================

struct C$KphpJobWorkerRequest : public job_worker_impl_::SendableBase {
  C$KphpJobWorkerRequest *virtual_builtin_clone() const noexcept override = 0;
};

// === KphpJobWorkerResponse ======================================================================

struct C$KphpJobWorkerResponse : public job_worker_impl_::SendableBase {
  C$KphpJobWorkerResponse *virtual_builtin_clone() const noexcept override = 0;
};

// === KphpJobWorkerResponseError =================================================================

struct C$KphpJobWorkerResponseError : public refcountable_polymorphic_php_classes<C$KphpJobWorkerResponse> {
  string error;
  int64_t error_code;

  const char *get_class() const noexcept override {
    return "KphpJobWorkerResponseError";
  }

  int32_t get_hash() const noexcept override {
    std::string_view name_view{get_class()};
    return static_cast<int32_t>(vk::murmur_hash<uint32_t>(name_view.data(), name_view.size()));
  }

  size_t virtual_builtin_sizeof() const noexcept override {
    return sizeof(*this);
  }

  C$KphpJobWorkerResponseError *virtual_builtin_clone() const noexcept override {
    return new C$KphpJobWorkerResponseError{*this};
  }
};

inline class_instance<C$KphpJobWorkerResponseError> f$KphpJobWorkerResponseError$$__construct(class_instance<C$KphpJobWorkerResponseError> v$this) noexcept {
  return v$this;
}

inline string f$KphpJobWorkerResponseError$$getError(const class_instance<C$KphpJobWorkerResponseError> &v$this) noexcept {
  return v$this.get()->error;
}

inline int64_t f$KphpJobWorkerResponseError$$getErrorCode(const class_instance<C$KphpJobWorkerResponseError> &v$this) noexcept {
  return v$this.get()->error_code;
}
