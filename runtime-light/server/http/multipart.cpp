// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/server/http/multipart.h"
#include "runtime-light/stdlib/file/resource.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/server/http/http-server-state.h"
#include "runtime-light/stdlib/math/random-functions.h"

#include <string_view>
#include <cstdio>

#include "runtime-common/core/runtime-core.h"
#include "common/algorithms/string-algorithms.h"

namespace {

constexpr int8_t TMP_FILENAME_LENGTH = 10;
constexpr std::string_view TMP_DIR = "/tmp/";

constexpr std::string_view HEADER_CONTENT_DISPOSITION_FORM_DATA = "form-data;";
constexpr std::string_view MULTIPART_BOUNDARY_EQ = "boundary=";

struct header {
  header() = delete;

  private:
  header(std::string_view name_view, std::string_view value_view) : name{name_view}, value{value_view} {}

  public:
  std::string_view name;
  std::string_view value;

  static std::optional<header> create(std::string_view header_string) noexcept {
    auto [name_view, value_view]{vk::split_string_view(header_string, ':')};
    if (name_view.size() + value_view.size() + 1 != header_string.size()) [[unlikely]] {
      return std::nullopt;
    }
    if (name_view.empty() || value_view.empty()) {
      return std::nullopt;
    }
    return header(name_view, value_view.substr(1));
  }

  bool name_is(std::string_view s) {
    const auto lower_name{name | std::views::take(s.size()) |
                              std::views::transform([](auto c) noexcept { return std::tolower(c, std::locale::classic()); })};
    return std::ranges::equal(lower_name, s);
  }
};

// Represents one attribute from Content-Disposition header. 
// For example, a typically file field will have two attributes: 
// 1) attr = "name", value = "avatar"
// 2) attr = "filename", value = "my_avatar.png"
struct partAttr {
  const std::string_view attr;
  const std::string_view value;

  partAttr() = delete;
  partAttr(std::string_view attr_, std::string_view value_) : attr{attr_}, value{value_} {};
};

// Represents one part of multipart content
struct part {
  std::string_view name;
  std::string_view filename;
  std::string_view content_type;
  std::string_view data;
};

class MultipartBody {
  private:
    std::string_view body;
    std::string_view boundary;
  
    void addPost(const part &part, mixed &v$_POST);
    void addFile(const part &part, mixed &v$_FILES);
  
  public:
    MultipartBody(std::string_view body_, std::string_view boundary_) 
      : body{body_}, boundary{boundary_} {}
    
    void parse_into(mixed &v$_POST, mixed &v$_FILES);
};

std::string_view trim_crlf(std::string_view s) {
  if (s.starts_with('\r')) {
    s = s.substr(1);
  }
  if (s.starts_with('\n')) {
    s = s.substr(1);
  }
  if (s.ends_with('\n')) {
    s = s.substr(0, s.size()-1);
  }
  if (s.ends_with('\r')) {
    s = s.substr(0, s.size()-1);
  }
  return s;
}

std::pair<std::optional<header>, std::string_view> parse_next_header(std::string_view s) {
  size_t lf{s.find('\n')};
  if (lf == std::string_view::npos) {
      return {std::nullopt, s};
  }
  std::string_view header_str{trim_crlf(s.substr(0, lf))};
  auto hdr{header::create(header_str)};
  s = s.substr(lf+1);
  return std::pair<std::optional<header>, std::string_view>(hdr, s);
}

auto parse_attrs(std::string_view header_value) {
  return std::views::split(header_value, ';') | std::views::transform([](auto part) {
    std::string_view part_view{vk::trim(std::string_view(part))};
    auto [name_view, value_view]{vk::split_string_view(part_view, '=')};
    if (value_view.size() >= 2 && value_view.starts_with('"') && value_view.ends_with('"')) {
      value_view = value_view.substr(1, value_view.size()-2);
    }
    return partAttr{name_view, value_view};
  });
}

void MultipartBody::parse_into(mixed &v$_POST, mixed &v$_FILES) {
  body = trim_crlf(body);
  if (!body.ends_with("--")) {
    return;
  }

  auto parts = std::views::split(body, boundary) | std::views::transform([](auto raw_part) noexcept -> std::optional<part> {
    std::string_view str{raw_part};
    if (!str.ends_with("--")) {
      return std::nullopt;
    }
    // cut "--" before next boundary
    str = str.substr(0, str.size() - 2);
    // trim \r\n
    str = trim_crlf(str);
    if (str.empty()) {
      return std::nullopt;
    }

    part part;
    while (true) {
      auto [maybe_header, new_str] = parse_next_header(str);
      str = new_str;
      if (!maybe_header) {
        break;
      }
      header header{maybe_header.value()};
      if (header.name_is(kphp::http::headers::CONTENT_DISPOSITION)) {
        if (!header.value.starts_with(HEADER_CONTENT_DISPOSITION_FORM_DATA)) {
          return std::nullopt;
        }
        auto attrs = parse_attrs(header.value);
        for (partAttr a : attrs) {
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
    if (!maybe_part) {
      continue;
    }
    part p{maybe_part.value()};
    if (p.name.empty()) {
      continue;
    }
    if (!p.filename.empty()) {
      addFile(p, v$_FILES);
    } else {
      addPost(p, v$_POST);
    }
  }
}

void MultipartBody::addPost(const part &part, mixed &v$_POST) {
  string name{part.name.data(), static_cast<string::size_type>(part.name.size())};
  v$_POST.set_value(name, string(part.data.data(), part.data.size()));
}

void MultipartBody::addFile(const part &part, mixed &v$_FILES) {
  //TODO: replace f$random_bytes because: 1) to avoid string allocation, 2) it may returns non-char bytes
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

namespace kphp::http {

void parse_multipart(std::string_view body, std::string_view boundary, mixed &v$_POST, mixed &v$_FILES) {
    MultipartBody mb{body, boundary};
    mb.parse_into(v$_POST, v$_FILES);
}

std::optional<std::string_view> parse_boundary(std::string_view content_type) {
  size_t pos{content_type.find(MULTIPART_BOUNDARY_EQ)};
  if (pos == std::string_view::npos) {
    return std::nullopt;
  }
  std::string_view res{content_type.substr(pos + MULTIPART_BOUNDARY_EQ.size())};
  if (res.size() >= 2 && res.starts_with('"') && res.ends_with('"')) {
      res = res.substr(1, res.size()-2);
  }
  return res;
}

} // namespace kphp::http
