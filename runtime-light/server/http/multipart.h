// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <string_view>

#include "runtime-common/core/runtime-core.h"

namespace kphp::http {

void parse_multipart(const std::string_view body, const std::string_view boundary, mixed &v$_POST, mixed &v$_FILES);

std::string_view parse_boundary(const std::string_view content_type);

} // namespace kphp::http
