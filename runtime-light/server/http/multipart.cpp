// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
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

struct header {
  const std::string_view name;
  const std::string_view value;

  static std::optional<header> create(std::string_view header_string) noexcept {
    auto [name_view, value_view]{vk::split_string_view(header_string, ':')};
    if (name_view.empty() || value_view.empty()) {
      return std::nullopt;
    }
    return header{.name = name_view, .value = value_view.substr(1)};
  }

  bool name_is(std::string_view header_name) noexcept {
    const auto lower_name{name | std::views::transform([](auto c) noexcept { return std::tolower(c, std::locale::classic()); })};
    const auto lower_header_name{header_name | std::views::transform([](auto c) noexcept { return std::tolower(c, std::locale::classic()); })};
    return std::ranges::equal(lower_name, lower_header_name);
  }
};

// Represents one attribute from Content-Disposition header.
// For example, a typically file field will have two attributes:
// 1) attr = "name", value = "avatar"
// 2) attr = "filename", value = "my_avatar.png"
struct part_attribute {
  const std::string_view attr;
  const std::string_view value;

  part_attribute(std::string_view attr_, std::string_view value_) noexcept
      : attr{attr_},
        value{value_} {};
};

// Represents one part of multipart content
struct part {
  std::string_view name;
  std::string_view filename;
  std::string_view content_type;
  std::string_view data;
};

class MultipartBody {
  std::string_view body;
  std::string_view boundary;

  void addPost(const part& part, mixed& v$_POST);
  void addFile(const part& part, mixed& v$_FILES);

public:
  MultipartBody(std::string_view body_, std::string_view boundary_)
      : body{body_},
        boundary{boundary_} {}

  void parse_into(mixed& v$_POST, mixed& v$_FILES);
};

std::string_view trim_crlf(std::string_view s) {
  if (s.starts_with('\r')) {
    s = s.substr(1);
  }
  if (s.starts_with('\n')) {
    s = s.substr(1);
  }
  if (s.ends_with('\n')) {
    s = s.substr(0, s.size() - 1);
  }
  if (s.ends_with('\r')) {
    s = s.substr(0, s.size() - 1);
  }
  return s;
}

std::pair<std::optional<header>, std::string_view> parse_next_header(std::string_view s) {
  size_t lf{s.find('\n')};
  size_t header_end{lf - 1};

  if (lf == std::string_view::npos) {
    return {std::nullopt, s};
  }

  if (s[header_end] == '\r') {
    header_end--;
  }

  auto hdr{header::create(s.substr(0, header_end + 1))};
  s = s.substr(lf + 1);
  return std::pair<std::optional<header>, std::string_view>(hdr, s);
}

auto parse_attrs(std::string_view header_value) {
  return std::views::split(header_value, ";") | std::views::transform([](auto part) {
           std::string_view part_view{vk::trim(std::string_view(part))};
           auto [name_view, value_view]{vk::split_string_view(part_view, '=')};
           if (value_view.size() >= 2 && value_view.starts_with('"') && value_view.ends_with('"')) {
             value_view = value_view.substr(1, value_view.size() - 2);
           }
           return part_attribute{name_view, value_view};
         });
}

void MultipartBody::parse_into(mixed& v$_POST, mixed& v$_FILES) {
  kphp::log::info("body {}", body);
  kphp::log::info("boundary {}", boundary);
  auto parts = std::views::split(body, boundary) | std::views::transform([](auto raw_part) noexcept -> std::optional<part> {
                 kphp::log::info("part of body {}", raw_part);
                 std::string_view str{raw_part};
                 if (!str.ends_with("--")) {
                   return std::nullopt;
                 }
                 // cut "--" before next boundary
                 str = str.substr(0, str.size() - 2);
                 // trim \r\n
                 str = trim_crlf(str);

                 part part;
                 while (true) {
                   auto [maybe_header, new_str] = parse_next_header(str);
                   if (!maybe_header) {
                     break;
                   }
                   str = new_str;
                   header header{maybe_header.value()};
                   kphp::log::info("header name {}", header.name);
                   if (header.name_is(kphp::http::headers::CONTENT_DISPOSITION)) {
                     if (!header.value.starts_with(HEADER_CONTENT_DISPOSITION_FORM_DATA)) {
                       return std::nullopt;
                     }
                     auto attrs = parse_attrs(header.value);
                     for (part_attribute a : attrs) {
                       if (a.attr.empty()) {
                         return std::nullopt;
                       }
                       if (a.attr == "name") {
                         part.name = a.value;
                       } else if (a.attr == "filename") {
                         part.filename = a.value;
                       }
                     }
                   } else if (header.name_is(kphp::http::headers::CONTENT_TYPE)) {
                     part.content_type = header.value;
                   }
                 }
                 part.data = str;
                 return part;
               });

  for (std::optional<part> maybe_part : parts) {
    kphp::log::info("process multipart part");
    if (!maybe_part) {
      continue;
    }
    kphp::log::info("process multipart non-null part");
    part p{maybe_part.value()};
    if (p.name.empty()) {
      continue;
    }
    kphp::log::info("process multipart non-empty part");
    if (!p.filename.empty()) {
      addFile(p, v$_FILES);
    } else {
      addPost(p, v$_POST);
    }
  }
}

void MultipartBody::addPost(const part& part, mixed& v$_POST) {
  string name{part.name.data(), static_cast<string::size_type>(part.name.size())};
  v$_POST.set_value(name, string(part.data.data(), part.data.size()));
}

void MultipartBody::addFile(const part& part, mixed& v$_FILES) {
  // TODO: replace f$random_bytes to avoid string allocation
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
  auto file_size_result = (*file).write({reinterpret_cast<const std::byte*>(part.data.data()), part.data.size()});
  if (file_size_result.has_value()) {
    file_size = file_size_result.value();
    if (file_size < part.data.size()) {
      kphp::log::warning("error write to tmp file: wrote {} bytes insted of {}", file_size, part.data.size());
      return;
    }
  } else {
    kphp::log::warning("error write to tmp file: errcode {}", file_size_result.error());
  }

  string name{part.name.data(), static_cast<string::size_type>(part.name.size())};

  if (part.name.ends_with("[]")) {
    mixed& file = v$_FILES[name.substr(0, name.size() - 2)];
    if (file_size == part.data.size()) {
      file[string("name")].push_back(string(part.filename.data(), part.filename.size()));
      file[string("type")].push_back(string(part.content_type.data(), part.content_type.size()));
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
    if (file_size == part.data.size()) {
      file.set_value(string("name"), string(part.filename.data(), part.filename.size()));
      file.set_value(string("type"), string(part.content_type.data(), part.content_type.size()));
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

namespace kphp::http::multipart {

void parse_multipart(std::string_view body, std::string_view boundary, mixed& v$_POST, mixed& v$_FILES) {
  MultipartBody mb{body, boundary};
  mb.parse_into(v$_POST, v$_FILES);
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

} // namespace kphp::http::multipart
