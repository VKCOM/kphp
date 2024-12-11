// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/file/file-resource.h"

namespace resource_impl_ {

inline constexpr std::string_view UDP_RESOURCE_PREFIX = "udp://";

struct UdpResourceWrapper : public ResourceWrapper {
  uint64_t stream_d{0};

  const char *get_class() const noexcept override {
    return R"(UdpResourceWrapper)";
  }

  task_t<int64_t> write(const std::string_view text) noexcept override;
  task_t<Optional<string>> get_contents() noexcept override {co_return false;}
  void flush() noexcept override {}
  void close() noexcept override;

  ~UdpResourceWrapper() {
    close();
  }
};

std::pair<class_instance<UdpResourceWrapper>, int32_t> open_udp_stream(const std::string_view scheme) noexcept;
} // namespace resource_impl_
