// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/server/http/multipart/details/parts-parsing.h"

namespace kphp::http::multipart::details {

void process_post_multipart(const kphp::http::multipart::details::part& part, mixed& post) noexcept;

void process_upload_multipart(const kphp::http::multipart::details::part& part, mixed& files) noexcept;

} // namespace kphp::http::multipart::details
