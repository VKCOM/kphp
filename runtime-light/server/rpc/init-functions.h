// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-light/tl/tl-functions.h"

namespace kphp::rpc {

void init_server(tl::K2InvokeRpc invoke_rpc) noexcept;

} // namespace kphp::rpc
