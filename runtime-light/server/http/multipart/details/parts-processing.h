// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/server/http/multipart/details/parts-parsing.h"

namespace kphp::http::multipart::details {

void process_post_multipart(const kphp::http::multipart::details::part& part, array<mixed>& post) noexcept;

void process_file_multipart(const kphp::http::multipart::details::part& part, array<mixed>& files) noexcept;

} // namespace kphp::http::multipart::details
