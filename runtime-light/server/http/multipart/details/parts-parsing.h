// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cstddef>
#include <locale>
#include <optional>
#include <ranges>
#include <string_view>
#include <utility>

#include "common/algorithms/string-algorithms.h"
#include "runtime-light/server/http/http-server-state.h"

namespace kphp::http::multipart::details {

constexpr std::string_view HEADER_CONTENT_DISPOSITION_FORM_DATA = "form-data;";

inline std::string_view trim_crlf(std::string_view sv) noexcept {
  if (sv.starts_with('\r')) {
    sv.remove_prefix(1);
  }
  if (sv.starts_with('\n')) {
    sv.remove_prefix(1);
  }

  if (sv.ends_with('\n')) {
    sv.remove_suffix(1);
  }
  if (sv.ends_with('\r')) {
    sv.remove_suffix(1);
  }
  return sv;
}

struct part_header {
  std::string_view name;
  std::string_view value;

  static std::optional<part_header> parse(std::string_view header) noexcept {
    auto [name_view, value_view]{vk::split_string_view(header, ':')};
    name_view = vk::trim(name_view);
    value_view = vk::trim(value_view);
    if (name_view.empty() || value_view.empty()) {
      return std::nullopt;
    }
    return part_header{name_view, value_view};
  }

  bool name_is(std::string_view header_name) const noexcept {
    const auto lower_name{name | std::views::transform([](auto c) noexcept { return std::tolower(c, std::locale::classic()); })};
    const auto lower_header_name{header_name | std::views::transform([](auto c) noexcept { return std::tolower(c, std::locale::classic()); })};
    return std::ranges::equal(lower_name, lower_header_name);
  }

private:
  part_header(std::string_view name, std::string_view value) noexcept
      : name(name),
        value(value) {}
};

inline auto parse_headers(std::string_view sv) noexcept {
  static constexpr std::string_view DELIM = "\r\n";
  return std::views::split(sv, DELIM) | std::views::transform([](auto raw_header) noexcept { return part_header::parse(std::string_view(raw_header)); }) |
         std::views::take_while([](auto header_opt) noexcept { return header_opt.has_value(); }) |
         std::views::transform([](auto header_opt) noexcept { return *header_opt; });
}

struct part_attribute {
  std::string_view name;
  std::string_view value;

  static std::optional<part_attribute> parse(std::string_view attribute) noexcept {
    auto [name_view, value_view]{vk::split_string_view(vk::trim(attribute), '=')};
    name_view = vk::trim(name_view);
    value_view = vk::trim(value_view);
    if (name_view.empty() || value_view.empty()) {
      return std::nullopt;
    }

    if (value_view.starts_with('"') && value_view.ends_with('"')) {
      value_view.remove_suffix(1);
      value_view.remove_prefix(1);
    }
    return part_attribute{name_view, value_view};
  }

private:
  part_attribute(std::string_view name, std::string_view value) noexcept
      : name(name),
        value(value) {}
};

inline auto parse_attrs(std::string_view header_value) noexcept {
  static constexpr std::string_view DELIM = ";";
  return std::views::split(header_value, DELIM) | std::views::transform([](auto part) noexcept { return part_attribute::parse(std::string_view(part)); }) |
         std::views::take_while([](auto attribute_opt) noexcept { return attribute_opt.has_value(); }) |
         std::views::transform([](auto attribute_opt) noexcept { return *attribute_opt; });
}

struct part {
  std::string_view name_attribute;
  std::optional<std::string_view> filename_attribute;
  std::optional<std::string_view> content_type;
  std::string_view body;

  static std::optional<part> parse(std::string_view part_view) noexcept {
    static constexpr std::string_view PART_BODY_DELIM = "\r\n\r\n";

    const size_t part_body_start{part_view.find(PART_BODY_DELIM)};
    if (part_body_start == std::string_view::npos) {
      return std::nullopt;
    }

    const std::string_view part_headers{part_view.substr(0, part_body_start)};
    const std::string_view part_body{part_view.substr(part_body_start + PART_BODY_DELIM.size())};

    std::optional<std::string_view> content_type{std::nullopt};
    std::optional<std::string_view> filename_attribute{std::nullopt};
    std::optional<std::string_view> name_attribute{std::nullopt};

    for (const auto& header : parse_headers(part_headers)) {
      if (header.name_is(kphp::http::headers::CONTENT_DISPOSITION)) {
        if (!header.value.starts_with(HEADER_CONTENT_DISPOSITION_FORM_DATA)) {
          return std::nullopt;
        }

        // skip first Content-Disposition: form-data;
        const size_t pos{header.value.find(';')};
        if (pos == std::string::npos) {
          return std::nullopt;
        }

        const std::string_view attributes{trim_crlf(header.value).substr(pos + 1)};
        for (const auto& attribute : parse_attrs(attributes)) {
          if (attribute.name == "name") {
            name_attribute = attribute.value;
          } else if (attribute.name == "filename") {
            filename_attribute = attribute.value;
          } else {
            // ignore unknown attribute
          }
        }
      } else if (header.name_is(kphp::http::headers::CONTENT_TYPE)) {
        content_type = trim_crlf(header.value);
      } else {
        // ignore unused header
      }
    }
    if (!name_attribute.has_value() || name_attribute->empty()) {
      return std::nullopt;
    }
    return part(*name_attribute, filename_attribute, content_type, part_body);
  }

private:
  part(std::string_view name_attribute, std::optional<std::string_view> filename_attribute, std::optional<std::string_view> content_type,
       std::string_view body) noexcept
      : name_attribute(name_attribute),
        filename_attribute(filename_attribute),
        content_type(content_type),
        body(body) {}
};

template<typename Delim>
auto parse_multipart_parts(std::string_view body, Delim&& delim) noexcept {
  return std::views::split(body, std::forward<Delim>(delim)) |
         std::views::filter([](auto raw_part) noexcept { return !std::string_view(raw_part).empty(); }) |
         std::views::transform([](auto raw_part) noexcept -> std::optional<part> { return part::parse(trim_crlf(std::string_view(raw_part))); }) |
         std::views::take_while([](auto part_opt) noexcept { return part_opt.has_value(); }) | std::views::transform([](auto part_opt) { return *part_opt; });
}

} // namespace kphp::http::multipart::details
