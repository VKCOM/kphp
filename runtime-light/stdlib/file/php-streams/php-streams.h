// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/file/file-resource.h"

namespace resource_impl_ {

inline constexpr std::string_view PHP_STREAMS_PREFIX = "php://";

inline constexpr std::string_view STDERR_NAME = "stderr";
inline constexpr std::string_view STDOUT_NAME = "stdout";
inline constexpr std::string_view STDIN_NAME = "stdin";

class_instance<ResourceWrapper> open_php_stream(const std::string_view scheme) noexcept;
} // namespace resource_impl_
