// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/streams/stream.h"

namespace kphp::http {

void init_server(kphp::component::stream&& request_stream, kphp::stl::vector<std::byte, kphp::memory::script_allocator>&& request) noexcept;

kphp::coro::task<> finalize_server() noexcept;

} // namespace kphp::http
