// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

inline constexpr std::string_view UDP_STREAMS_PREFIX = "udp://";

std::pair<uint64_t, int32_t> connect_to_host_by_udp(const string &hostname) noexcept;
