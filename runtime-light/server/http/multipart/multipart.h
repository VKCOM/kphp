// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <optional>
#include <string_view>

#include "runtime-light/server/http/multipart/details/parts-parsing.h"
#include "runtime-light/server/http/multipart/details/parts-processing.h"

namespace kphp::http::multipart {

constexpr std::string_view MULTIPART_BOUNDARY_EQ = "boundary=";

inline void process_multipart_content_type(std::string_view body, std::string_view boundary, PhpScriptBuiltInSuperGlobals& superglobals) noexcept {
  for (const auto& part : details::parse_multipart_parts(body, boundary)) {
    if (part.filename_attribute.has_value()) {
      details::process_file_multipart(part, superglobals.v$_FILES);
    } else {
      details::process_post_multipart(part, superglobals.v$_POST);
    }
  }
}

inline std::optional<std::string_view> extract_boundary(std::string_view content_type) noexcept {
  const size_t pos{content_type.find(MULTIPART_BOUNDARY_EQ)};
  if (pos == std::string_view::npos) {
    return std::nullopt;
  }

  std::string_view boundary_view{content_type.substr(pos + MULTIPART_BOUNDARY_EQ.size())};
  if (boundary_view.starts_with('"') && boundary_view.ends_with('"')) {
    boundary_view.remove_suffix(1);
    boundary_view.remove_prefix(1);
  }
  return boundary_view;
}

} // namespace kphp::http::multipart
