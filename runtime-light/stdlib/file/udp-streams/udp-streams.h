// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/file/file-resource.h"

namespace resource_impl_ {

inline constexpr std::string_view UDP_RESOURCE_PREFIX = "udp://";

std::pair<class_instance<ResourceWrapper>, int32_t> open_udp_stream(const std::string_view scheme) noexcept;
} // namespace resource_impl_
