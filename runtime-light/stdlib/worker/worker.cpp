//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/worker/worker.h"

#include "common/algorithms/hashes.h"
#include "common/wrappers/string_view.h"

const char *C$KphpJobWorkerResponseError::get_class() const noexcept {
  return R"(KphpJobWorkerResponseError)";
}

int C$KphpJobWorkerResponseError::get_hash() const noexcept {
  return static_cast<int32_t>(vk::std_hash(vk::string_view(C$KphpJobWorkerResponseError::get_class())));
}

size_t C$KphpJobWorkerResponseError::virtual_builtin_sizeof() const noexcept {
  return sizeof(*this);
}

C$KphpJobWorkerResponseError *C$KphpJobWorkerResponseError::virtual_builtin_clone() const noexcept {
  return nullptr; // TODO
}

class_instance<C$KphpJobWorkerResponseError> f$KphpJobWorkerResponseError$$__construct(const class_instance<C$KphpJobWorkerResponseError> &v$this) noexcept {
  return v$this;
}

string f$KphpJobWorkerResponseError$$getError(const class_instance<C$KphpJobWorkerResponseError> &v$this) noexcept {
  return v$this.get()->error;
}

int64_t f$KphpJobWorkerResponseError$$getErrorCode(const class_instance<C$KphpJobWorkerResponseError> &v$this) noexcept {
  return v$this.get()->error_code;
}
