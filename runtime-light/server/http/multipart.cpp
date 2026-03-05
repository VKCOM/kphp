// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstdint>
#include <cstdio>
#include <optional>
#include <ranges>
#include <string_view>
#include <utility>

#include "common/algorithms/string-algorithms.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/server/http/http-server-state.h"
#include "runtime-light/server/http/multipart.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/file/resource.h"
#include "runtime-light/stdlib/math/random-functions.h"

namespace {

constexpr int8_t TMP_FILENAME_LENGTH = 10;
constexpr std::string_view TMP_DIR = "/tmp/";

constexpr std::string_view HEADER_CONTENT_DISPOSITION_FORM_DATA = "form-data;";
constexpr std::string_view MULTIPART_BOUNDARY_EQ = "boundary=";

std::string_view trim_crlf(std::string_view sv) {
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
  const std::string_view name;
  const std::string_view value;

  static std::optional<part_header> parse(std::string_view header) noexcept {
    auto [name_view, value_view]{vk::split_string_view(header, ':')};
    if (name_view.empty() || value_view.empty()) {
      return std::nullopt;
    }
    return part_header{name_view, value_view.substr(1)};
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

auto parse_headers(std::string_view sv) noexcept {
  static constexpr std::string_view DELIM = "\r\n";
  return std::views::split(sv, DELIM) | std::views::transform([](auto raw_header) { return part_header::parse(std::string_view(raw_header)); }) |
         std::views::take_while([](auto header_opt) { return header_opt.has_value(); }) | std::views::transform([](auto header_opt) { return *header_opt; });
}

struct part_attribute {
  const std::string_view name;
  const std::string_view value;

  static std::optional<part_attribute> parse(std::string_view attribute) noexcept {
    auto [name_view, value_view]{vk::split_string_view(vk::trim(attribute), '=')};
    if (name_view.empty() || value_view.empty()) {
      return std::nullopt;
    }
    // todo assert "value"
    if (value_view.size() >= 2 && value_view.starts_with('"') && value_view.ends_with('"')) {
      value_view = value_view.substr(1, value_view.size() - 2);
    }
    return part_attribute{name_view, value_view};
  }

private:
  part_attribute(std::string_view name, std::string_view value) noexcept
      : name(name),
        value(value) {}
};

auto parse_attrs(std::string_view header_value) noexcept {
  static constexpr std::string_view DELIM = ";";
  return std::views::split(header_value, DELIM) | std::views::transform([](auto part) { return part_attribute::parse(std::string_view(part)); }) |
         std::views::take_while([](auto attribute_opt) { return attribute_opt.has_value(); }) |
         std::views::transform([](auto attribute_opt) { return *attribute_opt; });
}

struct part {
  std::string_view name_attribute;
  std::optional<std::string_view> filename_attribute;
  std::optional<std::string_view> content_type;
  std::string_view body;

  static std::optional<part> parse(std::string_view part_view) {
    static constexpr std::string_view PART_BODY_DELIM = "\r\n\r\n";

    const size_t part_body_start{part_view.find(PART_BODY_DELIM)};
    if (part_body_start == std::string_view::npos) {
      return std::nullopt;
    }

    const std::string_view part_headers{part_view.substr(0, part_body_start)};
    const std::string_view part_body{part_view.substr(part_body_start + PART_BODY_DELIM.size())};

    part part;
    for (const auto& header : parse_headers(part_headers)) {
      if (header.name_is(kphp::http::headers::CONTENT_DISPOSITION)) {
        if (!header.value.starts_with(HEADER_CONTENT_DISPOSITION_FORM_DATA)) {
          return std::nullopt;
        }

        size_t pos = header.value.find(';');
        std::string_view attributes = header.value.substr(pos + 1, header.value.find('\n') - pos);
        kphp::log::info("header.value {}", attributes);
        for (auto attribute : parse_attrs(attributes)) {
          kphp::log::info("attribute with name {}", attribute.name);
          if (attribute.name == "name") {
            part.name_attribute = attribute.value;
          } else if (attribute.name == "filename") {
            part.filename_attribute = attribute.value;
          } else {
            // ignore unknown attribute
          }
        }
      } else if (header.name_is(kphp::http::headers::CONTENT_TYPE)) {
        part.content_type = header.value;
      } else {
        // ignore unused header
      }
    }
    part.body = part_body;

    return part;
  }

private:

};

auto parse_parts(std::string_view body, std::string_view boundary) noexcept {
  return std::views::split(body, std::views::join(std::array{std::string_view{"--"}, boundary})) |
         std::views::filter([](auto raw_part) { return !std::string_view(raw_part).empty(); }) |
         std::views::transform([](auto raw_part) noexcept -> std::optional<part> { return part::parse(trim_crlf(std::string_view(raw_part))); }) |
         std::views::take_while([](auto part_opt) { return part_opt.has_value(); }) | std::views::transform([](auto part_opt) { return *part_opt; });
}

void addPost(const part& part, mixed& v$_POST) {
  kphp::log::info("addPost");
  string name{part.name_attribute.data(), static_cast<string::size_type>(part.name_attribute.size())};
  v$_POST.set_value(name, string(part.body.data(), part.body.size()));
}

void addFile(const part& part, mixed& v$_FILES) {
  //   TODO: replace f$random_bytes to avoid string allocation
  Optional<string> rand_str{f$random_bytes(TMP_FILENAME_LENGTH)};

  if (!rand_str.has_value()) {
    kphp::log::warning("error generating random_bytes for tmp file");
    return;
  }

  string tmp_name_str{TMP_DIR.data(), TMP_DIR.size()};
  tmp_name_str.append(rand_str.val());
  std::string_view tmp_name{tmp_name_str.c_str(), tmp_name_str.size()};

  auto file{kphp::fs::file::open(tmp_name, "w")};
  if (!file) [[unlikely]] {
    kphp::log::warning("error opening tmp file {}: error code -> {}", tmp_name, file.error());
    return;
  }

  int file_size{0};
  auto file_size_result = (*file).write({reinterpret_cast<const std::byte*>(part.body.data()), part.body.size()});
  if (file_size_result.has_value()) {
    file_size = file_size_result.value();
    if (file_size < part.body.size()) {
      kphp::log::warning("error write to tmp file: wrote {} bytes insted of {}", file_size, part.body.size());
      return;
    }
  } else {
    kphp::log::warning("error write to tmp file: errcode {}", file_size_result.error());
  }

  string name{part.name_attribute.data(), static_cast<string::size_type>(part.name_attribute.size())};

  if (part.name_attribute.ends_with("[]")) {
    mixed& file = v$_FILES[name.substr(0, name.size() - 2)];
    if (file_size == part.body.size()) {
      file[string("name")].push_back(string(part.filename_attribute.value().data(), part.filename_attribute.value().size()));
      file[string("type")].push_back(string(part.content_type.value_or("").data(), part.content_type.value_or("").size()));
      file[string("size")].push_back(file_size);
      file[string("tmp_name")].push_back(string(tmp_name.data(), tmp_name.size()));
      file[string("error")].push_back(0);
    } else {
      file[string("name")].push_back(string());
      file[string("type")].push_back(string());
      file[string("size")].push_back(0);
      file[string("tmp_name")].push_back(string());
      file[string("error")].push_back(-file_size);
    }
  } else {
    mixed& file = v$_FILES[name];
    if (file_size == part.body.size()) {
      file.set_value(string("name"), string(part.filename_attribute.value().data(), part.filename_attribute.value().size()));
      file.set_value(string("type"), string(part.content_type.value_or("").data(), part.content_type.value_or("").size()));
      file.set_value(string("size"), file_size);
      file.set_value(string("tmp_name"), string(tmp_name.data(), tmp_name.size()));
      file.set_value(string("error"), 0);
    } else {
      file.set_value(string("size"), 0);
      file.set_value(string("tmp_name"), string());
      file.set_value(string("error"), -file_size);
    }
  }
}

} // namespace

namespace kphp::http {

void process_multipart_content_type(std::string_view body, std::string_view boundary, PhpScriptBuiltInSuperGlobals& superglobals) noexcept {
  kphp::log::info("body {}", body);
  kphp::log::info("boundary {}", boundary);
  for (auto part : parse_parts(body, boundary)) {
    kphp::log::info("process multipart name_attribute {}", part.name_attribute);
    if (part.name_attribute.empty()) {
      continue;
    }

    if (part.filename_attribute.has_value()) {
      addFile(part, superglobals.v$_FILES);
    } else {
      addPost(part, superglobals.v$_POST);
    }
  }
}

std::optional<std::string_view> extract_boundary(std::string_view content_type) noexcept {
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

} // namespace kphp::http
