// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <expected>
#include <ranges>
#include <string_view>

#include "runtime-light/core/globals/php-script-globals.h"
#include "runtime-light/server/http/multipart/details/parts-parsing.h"
#include "runtime-light/server/http/multipart/details/parts-processing.h"

namespace kphp::http::multipart {

namespace details {
inline constexpr std::string_view MULTIPART_BOUNDARY_EQ = "boundary=";
inline constexpr size_t BOUNDARY_MAX_SIZE = 70;

inline constexpr std::string_view ERROR_NO_BOUNDARY = "boundary not found in multipart content type";
inline constexpr std::string_view ERROR_EMPTY_BOUNDARY = "boundary is empty in multipart content type";
inline constexpr std::string_view ERROR_BOUNDARY_TOO_LONG = "boundary exceeds max size in multipart content type";

inline std::expected<std::string_view, std::string_view> extract_boundary(std::string_view content_type) noexcept {
  auto boundary_match{std::ranges::search(content_type, details::MULTIPART_BOUNDARY_EQ)};
  if (boundary_match.empty()) {
    return std::unexpected{details::ERROR_NO_BOUNDARY};
  }
  auto after{std::string_view{boundary_match.end(), content_type.end()}};
  auto boundary_view{std::string_view{after.begin(), std::ranges::find(after, ';')}};
  if (boundary_view.starts_with('"') && boundary_view.ends_with('"') && boundary_view.size() > 1) {
    boundary_view.remove_prefix(1);
    boundary_view.remove_suffix(1);
  }
  if (boundary_view.empty()) {
    return std::unexpected{details::ERROR_EMPTY_BOUNDARY};
  }
  if (boundary_view.size() > BOUNDARY_MAX_SIZE) {
    return std::unexpected{details::ERROR_BOUNDARY_TOO_LONG};
  }
  return boundary_view;
}

} // namespace details

inline std::expected<void, std::string_view> process_multipart_content_type(std::string_view content_type, std::string_view body,
                                                                            PhpScriptBuiltInSuperGlobals& superglobals) noexcept {
  auto boundary_res{details::extract_boundary(content_type)};
  if (!boundary_res.has_value()) {
    return std::unexpected{boundary_res.error()};
  }
  auto boundary_parts{std::array{std::string_view{"--"}, *boundary_res}};
  auto delim{std::views::join(boundary_parts)};
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
