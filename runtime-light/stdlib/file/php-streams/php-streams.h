// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/file/file-resource.h"

namespace resource_impl_ {

inline constexpr std::string_view PHP_STREAMS_PREFIX = "php://";

struct PhpResourceWrapper : public ResourceWrapper {
  uint64_t stream_d{0};

  const char *get_class() const noexcept override {
    return R"(PhpResourceWrapper)";
  }

  virtual task_t<int64_t> write(const std::string_view text) override;
  virtual task_t<Optional<string>> get_contents() override;
  virtual void flush() override {}
  virtual void close() override {
    /*
     * PHP support multiple opening/closing operations on standard IO streams.
     * */
  }

  ~PhpResourceWrapper() {
    close();
  }
};

class_instance<PhpResourceWrapper> open_php_stream(const std::string_view scheme) noexcept;
} // namespace resource_impl_
