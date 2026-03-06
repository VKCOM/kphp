// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <optional>
#include <string_view>

#include "runtime-light/server/http/multipart/details/parts-parsing.h"
#include "runtime-light/server/http/multipart/details/parts-processing.h"

namespace kphp::http::multipart {

constexpr std::string_view MULTIPART_BOUNDARY_EQ = "boundary=";

inline void process_multipart_content_type(std::string_view body, std::string_view boundary, PhpScriptBuiltInSuperGlobals& superglobals) noexcept {
  for (auto part : details::parse_multipart_parts(body, boundary)) {
    kphp::log::info("process multipart name_attribute {}", part.name_attribute);
    if (part.filename_attribute.has_value()) {
      details::process_upload_multipart(part, superglobals.v$_FILES);
    } else {
      details::process_post_multipart(part, superglobals.v$_POST);
    }
  }
}

inline std::optional<std::string_view> extract_boundary(std::string_view content_type) noexcept {
  size_t pos{content_type.find(MULTIPART_BOUNDARY_EQ)};
  if (pos == std::string_view::npos) {
    return std::nullopt;
  }
  // todo assert "body"
  std::string_view res{content_type.substr(pos + MULTIPART_BOUNDARY_EQ.size())};
  if (res.size() >= 2 && res.starts_with('"') && res.ends_with('"')) {
    res = res.substr(1, res.size() - 2);
  }
  return res;
}

} // namespace kphp::http::multipart
