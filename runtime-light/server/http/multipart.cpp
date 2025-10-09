#include "runtime-light/server/http/multipart.h"
#include "runtime-light/stdlib/file/resource.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

#include <string_view>
#include <cstdio>

#include "runtime-common/core/core-types/decl/string_decl.inl"
#include "runtime-common/core/core-types/decl/mixed_decl.inl"

namespace {

constexpr std::string_view HEADER_CONTENT_DISPOSITION = "Content-Disposition";
constexpr std::string_view HEADER_CONTENT_TYPE = "Content-Type";
constexpr std::string_view HEADER_CONTENT_DISPOSITION_FORM_DATA = "form-data;";
constexpr std::string_view MULTIPART_BOUNDARY_EQ = "boundary=";

struct Header {
  std::string_view header_str, name, value;
  
  Header() {}
  Header(const std::string_view &header_str_) 
    : header_str{header_str_} {}

  void parse() {
    char delim = ':';
    size_t delim_pos = header_str.find(delim);
    name = header_str.substr(0, delim_pos);
    value = header_str.substr(delim_pos+2); // +1 for ':' and +1 for ' '
  }
};

// Represents one attribute from Content-Desposition header. 
// For example, a typically file field will have two attributes: 
// 1) attr = "name", value = "avatar"
// 2) attr = "filename", value = "my_avatar.png"
struct PartAttr {
  std::string_view attr, value;

  PartAttr() {};
  PartAttr(const std::string_view &attr_, const std::string_view &value_) : attr{attr_}, value{value_} {};
};

// Represents a parser of Content-Desposition header string.
struct AttrParser {
  private:
    std::string_view header;
    size_t pos{0};

  public:
    AttrParser(const std::string_view &header_) : header{header_} {}
    PartAttr next_attr();
    bool end() {
      return pos >= header.size();
    }
  
  private:
    void markEnd() {
      pos = header.size();
    } 
};

PartAttr AttrParser::next_attr() {
  if (pos == 0) {
    if (header.find(HEADER_CONTENT_DISPOSITION_FORM_DATA) != pos) {
      markEnd();
      return {};
    }
    pos += HEADER_CONTENT_DISPOSITION_FORM_DATA.size();
  }
  
  size_t 
    attrStart = header.find_first_not_of(' ', pos),
    attrEnd,
    valStart,
    valEnd;

  if (attrStart == std::string_view::npos) {
    markEnd();
    return {};
  }
  
  size_t eqi = header.find('=', attrStart);
  if (eqi == std::string_view::npos) {
    markEnd();
    return {};
  }

  attrEnd = eqi-1;
  valStart = eqi+1;
  pos = header.find(";", valStart);

  if (pos == std::string_view::npos) {
    valEnd = header.size()-1;
  } else {
    valEnd = pos-1;
    pos++;
  }
  if (header[valStart] == '"' && header[valEnd] == '"') {
    valStart++;
    valEnd--;
  }

  return {header.substr(attrStart, attrEnd-attrStart+1), header.substr(valStart, valEnd-valStart+1)};
}

// Represents one part of multipart content
struct Part {
  std::string_view name, filename, content_type, data;
};

struct MultipartBody {
  private:

    std::string_view body, boundary;
    size_t pos;
  
    Part next_part();
    void addPost(const Part &part, mixed &v$_POST);
    void addFile(const Part &part, mixed &v$_FILES);

    Header next_header();
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

    MultipartBody(const std::string_view &body_, const std::string_view &boundary_) 
      : body{body_}, boundary{boundary_}, pos{0} {}
    
    void parse_into(mixed &v$_POST, mixed &v$_FILES);
};

Part MultipartBody::next_part() {
  Part part;

  if (pos == 0) {
    skip_boundary();
    skip_crlf();
  }

  do {
    Header header = next_header();
    if (header.value.empty()) {
      markEnd();
      return {};
    }
    
    if (header.name == HEADER_CONTENT_DISPOSITION) {
      AttrParser parser{header.value};
      while (!parser.end()) {
        PartAttr pa = parser.next_attr();
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
    } else if (header.name == HEADER_CONTENT_TYPE) {
      part.content_type = header.value;
    }
  } while (!is_crlf());

  skip_crlf();
  part.data = parse_data();
  skip_boundary();
  skip_crlf();
  return part;
}

Header MultipartBody::next_header() {
  size_t 
    lf = body.find('\n', pos), 
    header_end = lf-1;
  
  if (lf == std::string_view::npos) {
      return {};
  }
  
  if (body[header_end] == '\r') {
    header_end--;
  }

  Header header{body.substr(pos, header_end-pos+1)};
  header.parse();

  pos = lf + 1;
  return header;
}

std::string_view MultipartBody::parse_data() {
  size_t 
    data_start = pos,
    data_end = pos = body.find(boundary, data_start);

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
    Part part = next_part();
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

void MultipartBody::addPost(const Part &part, mixed &v$_POST) {
  string name{part.name.data(), static_cast<string::size_type>(part.name.size())};
  v$_POST.set_value(name, string(part.data.data(), part.data.size()));
}

void MultipartBody::addFile(const Part &part, mixed &v$_FILES) {
  std::string_view tmp_name = std::tmpnam(nullptr);
  auto file{kphp::fs::file::open(tmp_name, "w")};
  if (!file) [[unlikely]] {
    kphp::log::warning("error opening tmp file: error code -> {}", file.error());
    return;
  }

  int file_size;
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
    if (file_size >= 0) {
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
    if (file_size >= 0) {
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

void parse_multipart(const std::string_view &body, const std::string_view &boundary, mixed &v$_POST, mixed &v$_FILES) {
    MultipartBody mb{body, boundary};
    mb.parse_into(v$_POST, v$_FILES);
}

std::string_view parse_boundary(const std::string_view &content_type) {
  size_t pos = content_type.find(MULTIPART_BOUNDARY_EQ);
  if (pos != std::string_view::npos) {
    std::string_view res = content_type.substr(pos + MULTIPART_BOUNDARY_EQ.size());
    if (res[0] == '"' && res[res.size()-1] == '"') {
        res = res.substr(1, res.size()-2);
    }
    return res;
  }
  return {};
}

} // namespace kphp::http