// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/tl/tl-functions.h"

namespace kphp {

namespace http {

void init_server(tl::K2InvokeHttp &&invoke_http) noexcept;

kphp::coro::task<> finalize_server(const string_buffer &output) noexcept;

} // namespace http

} // namespace kphp
