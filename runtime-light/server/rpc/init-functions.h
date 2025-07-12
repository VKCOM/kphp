// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-light/streams/stream.h"
#include "runtime-light/tl/tl-core.h"

namespace kphp::rpc {

void init_server(kphp::component::stream request_stream, tl::TLBuffer& tlb) noexcept;

} // namespace kphp::rpc
