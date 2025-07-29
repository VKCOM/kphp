// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-light/coroutine/task.h"
#include "runtime-light/streams/stream.h"
#include "runtime-light/tl/tl-core.h"

namespace kphp::http {

void init_server(kphp::component::stream request_stream, tl::TLBuffer& tlb) noexcept;

kphp::coro::task<> finalize_server() noexcept;

} // namespace kphp::http
