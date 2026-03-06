// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/server/http/multipart/details/parts-processing.h"

#include <cstddef>
#include <string_view>

#include "runtime-common/core/core-types/decl/optional.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/server/url-functions.h"
#include "runtime-light/server/http/multipart/details/parts-parsing.h"
#include "runtime-light/stdlib/file/resource.h"
#include "runtime-light/stdlib/math/random-functions.h"

namespace {

constexpr std::string_view CONTENT_TYPE_APP_FORM_URLENCODED = "application/x-www-form-urlencoded";

constexpr int8_t TMP_FILENAME_LENGTH = 10;
constexpr std::string_view TMP_DIR = "/tmp/";

constexpr std::string_view DEFAULT_CONTENT_TYPE = "text/plain";

constexpr int32_t UPLOAD_ERR_OK = 0;
constexpr int32_t UPLOAD_ERR_INI_SIZE = 1;  // unused in kphp
constexpr int32_t UPLOAD_ERR_FORM_SIZE = 2; // todo support header max-file-size
constexpr int32_t UPLOAD_ERR_PARTIAL = 3;
constexpr int32_t UPLOAD_ERR_NO_FILE = 4;
constexpr int32_t UPLOAD_ERR_NO_TMP_DIR = 6; // todo support check tmp dir
constexpr int32_t UPLOAD_ERR_CANT_WRITE = 7;
constexpr int32_t UPLOAD_ERR_EXTENSION = 8; // unused in kphp

} // namespace

namespace kphp::http::multipart::details {

void process_post_multipart(const kphp::http::multipart::details::part& part, mixed& post) noexcept {
  const string name{part.name_attribute.data(), static_cast<string::size_type>(part.name_attribute.size())};
  const string body{part.body.data(), static_cast<string::size_type>(part.body.size())};
  if (part.content_type.has_value() && (*part.content_type) == CONTENT_TYPE_APP_FORM_URLENCODED) {
    f$parse_str(body, post[name]);
  } else {
    post.set_value(name, string(part.body.data(), part.body.size()));
  }
}

void process_upload_multipart(const kphp::http::multipart::details::part& part, mixed& files) noexcept {
  //   TODO: replace f$random_bytes to avoid string allocation
  Optional<string> rand_str{f$random_bytes(TMP_FILENAME_LENGTH)};

  if (!rand_str.has_value()) [[unlikely]] {
    //    kphp::log::warning("error generating random_bytes for tmp file");
    return;
  }

  string tmp_name_str{TMP_DIR.data(), TMP_DIR.size()};
  tmp_name_str.append(rand_str.val());
  std::string_view tmp_name{tmp_name_str.c_str(), tmp_name_str.size()};

  auto file_res{kphp::fs::file::open(tmp_name, "w")};
  int32_t error_code{UPLOAD_ERR_OK};
  size_t file_size{};
  if (file_res.has_value()) {
    const auto written_res{(*file_res).write({reinterpret_cast<const std::byte*>(part.body.data()), part.body.size()})};
    if (written_res.has_value()) {
      file_size = *written_res;
      if (file_size < part.body.size()) {
        error_code = UPLOAD_ERR_PARTIAL;
      }
    } else {
      *file_res->close();
      error_code = UPLOAD_ERR_CANT_WRITE;
    }

  } else {
    error_code = UPLOAD_ERR_NO_FILE;
  }

  kphp::log::assertion(part.filename_attribute.has_value());

  const string name{part.name_attribute.data(), static_cast<string::size_type>(part.name_attribute.size())};
  if (part.name_attribute.ends_with("[]")) {
    mixed& file = files[name.substr(0, name.size() - 2)];
    if (error_code != UPLOAD_ERR_OK) {
      file[string("name")].push_back(string());
      file[string("type")].push_back(string());
      file[string("size")].push_back(0);
      file[string("tmp_name")].push_back(string());
      file[string("error")].push_back(error_code);
    } else {
      file[string("name")].push_back(string((*part.filename_attribute).data(), (*part.filename_attribute).size()));
      file[string("type")].push_back(string(part.content_type.value_or(DEFAULT_CONTENT_TYPE).data(), part.content_type.value_or(DEFAULT_CONTENT_TYPE).size()));
      file[string("size")].push_back(static_cast<int64_t>(file_size));
      file[string("tmp_name")].push_back(string(tmp_name.data(), tmp_name.size()));
      file[string("error")].push_back(0);
    }
  } else {
    mixed& file = files[name];
    if (error_code != UPLOAD_ERR_OK) {
      file.set_value(string("size"), 0);
      file.set_value(string("tmp_name"), string());
      file.set_value(string("error"), error_code);
    } else {
      file.set_value(string("name"), string((*part.filename_attribute).data(), (*part.filename_attribute).size()));
      file.set_value(string("type"), string(part.content_type.value_or(DEFAULT_CONTENT_TYPE).data(), part.content_type.value_or(DEFAULT_CONTENT_TYPE).size()));
      file.set_value(string("size"), static_cast<int64_t>(file_size));
      file.set_value(string("tmp_name"), string(tmp_name.data(), tmp_name.size()));
      file.set_value(string("error"), 0);
    }
  }
}
} // namespace kphp::http::multipart::details
