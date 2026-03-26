// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstddef>
#include <expected>
#include <optional>
#include <ranges>
#include <string_view>

#include "runtime-light/core/globals/php-script-globals.h"
#include "runtime-light/server/http/multipart/details/parts-parsing.h"
#include "runtime-light/server/http/multipart/details/parts-processing.h"

namespace kphp::http::multipart {

namespace details {
inline constexpr std::string_view MULTIPART_BOUNDARY_EQ = "boundary=";
inline constexpr size_t BOUNDARY_MAX_SIZE = 70;

inline std::optional<std::string_view> extract_boundary(std::string_view content_type) noexcept {
  const size_t boundary_start{content_type.find(details::MULTIPART_BOUNDARY_EQ)};
  if (boundary_start == std::string_view::npos) {
    return std::nullopt;
  }

  size_t boundary_end{content_type.find(';', boundary_start)};
  if (boundary_end == std::string_view::npos) {
    boundary_end = content_type.size();
  }

  std::string_view boundary_view{
      content_type.substr(boundary_start + details::MULTIPART_BOUNDARY_EQ.size(), boundary_end - boundary_start - details::MULTIPART_BOUNDARY_EQ.size())};
  if (boundary_view.starts_with('"') && boundary_view.ends_with('"') && boundary_view.size() > 1) {
    boundary_view.remove_prefix(1);
    boundary_view.remove_suffix(1);
  }

  if (boundary_view.empty() || boundary_view.size() > BOUNDARY_MAX_SIZE) {
    return std::nullopt;
  }
  return boundary_view;
}

} // namespace details

inline std::expected<void, std::string_view> process_multipart_content_type(std::string_view content_type, std::string_view body,
                                                                            PhpScriptBuiltInSuperGlobals& superglobals) noexcept {
  auto boundary_opt{details::extract_boundary(content_type)};
  if (!boundary_opt.has_value()) {
    return std::unexpected{"cannot extract boundary in multipart content type"};
  }
  const auto boundary_parts{std::array{std::string_view{"--"}, *boundary_opt}};
  const auto delim{std::views::join(boundary_parts)};
  for (const auto& part : details::parse_multipart_parts(body, delim)) {
    if (part.filename_attribute.has_value()) {
      details::process_file_multipart(part, superglobals.v$_FILES.as_array());
    } else {
      details::process_post_multipart(part, superglobals.v$_POST.as_array());
    }
  }
  return {};
}

} // namespace kphp::http::multipart
