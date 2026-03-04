// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <optional>
#include <string_view>

#include "runtime-common/core/runtime-core.h"

namespace kphp::http::multipart {

void parse_multipart(std::string_view body, std::string_view boundary, mixed& v$_POST, mixed& v$_FILES);

std::optional<std::string_view> extract_boundary(std::string_view content_type) noexcept;

} // namespace kphp::http::multipart
