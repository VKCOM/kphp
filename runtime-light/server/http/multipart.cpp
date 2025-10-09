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

const int TMP_FILENAME_LENGTH = 10;

constexpr std::string_view HEADER_CONTENT_DISPOSITION_FORM_DATA = "form-data;";
constexpr std::string_view MULTIPART_BOUNDARY_EQ = "boundary=";

struct header {
  std::string_view header_view, name, value;
  
  header() = default;
  header(const std::string_view header_str_) : header_view{header_str_} {
    auto [name_view, value_view]{vk::split_string_view(header_view, ':')};
    if (name_view.size() + value_view.size() + 1 != header_view.size()) [[unlikely]] {
      return;
    }
    name = name_view;
    if (!value_view.empty()) {
      value = value_view.substr(1); // skip ' '
    }
  }

  bool is_valid() {
    return !name.empty() && !value.empty();
  }

  bool name_is(const std::string_view s) {
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
  std::string_view attr, value;

  partAttr() = default;
  partAttr(const std::string_view attr_, const std::string_view value_) : attr{attr_}, value{value_} {};
};

// Represents a parser of Content-Disposition header string.
struct attrParser {
  private:
    std::string_view header;
    size_t pos{0};

  public:
    attrParser(const std::string_view header_) : header{header_} {}
    partAttr next_attr();
    bool end() {
      return pos >= header.size();
    }
  
  private:
    void markEnd() {
      pos = header.size();
    } 
};

partAttr attrParser::next_attr() {
  if (pos == 0) {
    if (!header.starts_with(HEADER_CONTENT_DISPOSITION_FORM_DATA)) {
      markEnd();
      return {};
    }
    pos = HEADER_CONTENT_DISPOSITION_FORM_DATA.size();
  }

  if (pos >= header.size()) {
    return {};
  }

  size_t end{header.find(';', pos)};
  if (end == std::string_view::npos) {
    end = header.size();
  }

  std::string_view part_view{vk::trim(header.substr(pos, end-pos))};
  auto [name_view, value_view]{vk::split_string_view(part_view, '=')};
  if (value_view.size() >= 2 && value_view.starts_with('"') && value_view.ends_with('"')) {
    value_view = value_view.substr(1, value_view.size()-2);
  }
  pos = end + 1;

  return {name_view, value_view};
}

// Represents one part of multipart content
struct part {
  std::string_view name, filename, content_type, data;
};

struct MultipartBody {
  private:

    std::string_view body, boundary;
    size_t pos;
  
    part next_part();
    void addPost(const part &part, mixed &v$_POST);
    void addFile(const part &part, mixed &v$_FILES);

    header next_header();
    std::string_view parse_data();
    
    // Returns true if current pos refers to one of \r, \n, \r\n
    bool is_crlf() {
      return body[pos] == '\r' || body[pos] == '\n' || (body[pos] == '\r' && body[pos+1] == '\n');
    }

    void skip_crlf() {
      if (body[pos] == '\r') {
          pos++;
      }
      if (body[pos] == '\n') {
          pos++;
      }
    }

    void skip_boundary() {
      if (pos == 0) {
        pos += 2;
      }
      pos += boundary.size();
      if (body[pos] == '-' && body[pos+1] == '-') {
          pos += 2;
      }
    }

    bool end() {
      return pos >= body.size();
    }

    void markEnd() {
        pos = body.size();
    }
  
  public:

    MultipartBody(const std::string_view body_, const std::string_view boundary_) 
      : body{body_}, boundary{boundary_}, pos{0} {}
    
    void parse_into(mixed &v$_POST, mixed &v$_FILES);
};

part MultipartBody::next_part() {
  part part;

  if (pos == 0) {
    skip_boundary();
    skip_crlf();
  }

  do {
    header header{next_header()};
    if (!header.is_valid()) {
      markEnd();
      return {};
    }
    
    if (header.name_is(kphp::http::headers::CONTENT_DISPOSITION)) {
      attrParser parser{header.value};
      while (!parser.end()) {
        partAttr pa{parser.next_attr()};
        if (pa.attr.empty()) {
          markEnd();
          return {};
        }
        if (pa.attr == "name") {
          part.name = pa.value;
        } else if (pa.attr == "filename") {
          part.filename = pa.value;
        }
      }
    } else if (header.name_is(kphp::http::headers::CONTENT_TYPE)) {
      part.content_type = header.value;
    }
  } while (!is_crlf());

  skip_crlf();
  part.data = parse_data();
  skip_boundary();
  skip_crlf();
  return part;
}

header MultipartBody::next_header() {
  size_t lf{body.find('\n', pos)};
  size_t header_end{lf-1};
  
  if (lf == std::string_view::npos) {
      return {};
  }
  
  if (body[header_end] == '\r') {
    header_end--;
  }

  header header{body.substr(pos, header_end-pos+1)};
  pos = lf + 1;
  return header;
}

std::string_view MultipartBody::parse_data() {
  size_t data_start{pos};
  size_t data_end{body.find(boundary, data_start)};
  pos = data_end;

  if (pos == std::string_view::npos) {
    return {};
  }

  if (body[data_end-1] != '-' || body[data_end-2] != '-') {
    return {};
  }
  data_end -= 2;
  if (body[data_end] == '\n') {
      data_end--;
  }
  if (body[data_end] == '\r') {
      data_end--;
  }

  if (data_end > data_start) {
    return body.substr(data_start, data_end-data_start-1);
  }

  return {};

}

void MultipartBody::parse_into(mixed &v$_POST, mixed &v$_FILES) {
  while (!end()) {
    part part{next_part()};
    if (part.name.empty()) {
      return;
    }
    
    if (!part.filename.empty()) {
      addFile(part, v$_FILES);
    } else {
      addPost(part, v$_POST);
    }
  }
}

void MultipartBody::addPost(const part &part, mixed &v$_POST) {
  string name{part.name.data(), static_cast<string::size_type>(part.name.size())};
  v$_POST.set_value(name, string(part.data.data(), part.data.size()));
}

void MultipartBody::addFile(const part &part, mixed &v$_FILES) {
  //TODO: replace f$random_bytes to avoid string allocation
  Optional<string> rand_str = f$random_bytes(TMP_FILENAME_LENGTH);
  
  if (!rand_str.has_value()) {
    kphp::log::warning("error generating random_bytes for tmp file");
    return;
  }
  std::string_view tmp_name{rand_str.val().c_str(), rand_str.val().size()};
  
  auto file{kphp::fs::file::open(tmp_name, "w")};
  if (!file) [[unlikely]] {
    kphp::log::warning("error opening tmp file: error code -> {}", file.error());
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

void parse_multipart(const std::string_view body, const std::string_view boundary, mixed &v$_POST, mixed &v$_FILES) {
    MultipartBody mb{body, boundary};
    mb.parse_into(v$_POST, v$_FILES);
}

std::string_view parse_boundary(const std::string_view content_type) {
  size_t pos{content_type.find(MULTIPART_BOUNDARY_EQ)};
  if (pos == std::string_view::npos) {
    return {};
  }
  std::string_view res{content_type.substr(pos + MULTIPART_BOUNDARY_EQ.size())};
  if (res.size() >= 2 && res.starts_with('"') && res.ends_with('"')) {
      res = res.substr(1, res.size()-2);
  }
  return res;
}

} // namespace kphp::http
