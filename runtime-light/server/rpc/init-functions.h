// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-light/streams/stream.h"

namespace kphp::rpc {

void init_server(kphp::component::stream request_stream) noexcept;

} // namespace kphp::rpc
