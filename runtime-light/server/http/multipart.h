// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <optional>
#include <string_view>

#include "runtime-light/core/globals/php-script-globals.h"

namespace kphp::http {

void process_multipart_content_type(std::string_view body, std::string_view boundary, PhpScriptBuiltInSuperGlobals& superglobals) noexcept;

std::optional<std::string_view> extract_boundary(std::string_view content_type) noexcept;

} // namespace kphp::http::multipart
