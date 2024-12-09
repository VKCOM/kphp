// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

inline constexpr std::string_view PHP_STREAMS_PREFIX = "php://";

uint64_t open_php_stream(const string &stream_name) noexcept;
