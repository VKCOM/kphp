// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"

using resource = mixed;

enum class ResourceKind {
  Udp,
  Php,
  Unknown,
};

struct ResourceWrapper : public refcountable_polymorphic_php_classes<may_be_mixed_base> {
  const ResourceKind kind;
  const string url;
  uint64_t stream_d{};

  ResourceWrapper(ResourceKind k, const string &u, uint64_t d)
    : kind(k)
    , url(u)
    , stream_d(d) {}

  const char *get_class() const noexcept override {
    return R"(ResourceWrapper)";
  }

  task_t<int64_t> write(const std::string_view text) const noexcept;
  Optional<string> get_contents() const noexcept;
  void flush() noexcept;
  void close() noexcept;

  ~ResourceWrapper() {
    close();
  }
};
