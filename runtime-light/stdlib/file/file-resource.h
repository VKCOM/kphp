// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"

using resource = mixed;

enum class ResourceKind {
  UdpResource,
  PhpResource,
  Unknown,
};

struct ResourceWrapper : public refcountable_polymorphic_php_classes<may_be_mixed_base> {
  uint64_t stream_d{};

  virtual task_t<int64_t> write(const std::string_view text) noexcept = 0 ;
  virtual task_t<Optional<string>> get_contents() noexcept = 0;
  virtual void flush() noexcept = 0;
  virtual void close() noexcept = 0;

  virtual ~ResourceWrapper() = 0;
};
