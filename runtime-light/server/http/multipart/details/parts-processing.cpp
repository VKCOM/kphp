// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/server/http/multipart/details/parts-processing.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <expected>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <unistd.h>
#include <utility>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/stdlib/server/url-functions.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/server/http/http-server-state.h"
#include "runtime-light/server/http/multipart/details/parts-parsing.h"
#include "runtime-light/state/component-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/file/resource.h"
#include "runtime-light/stdlib/math/random-functions.h"

namespace {

constexpr std::string_view CONTENT_TYPE_APP_FORM_URLENCODED = "application/x-www-form-urlencoded";

constexpr std::string_view DEFAULT_CONTENT_TYPE = "text/plain";

constexpr int32_t UPLOAD_ERR_OK = 0;
constexpr int32_t UPLOAD_ERR_PARTIAL = 3;
constexpr int32_t UPLOAD_ERR_NO_FILE = 4;
constexpr int32_t UPLOAD_ERR_CANT_WRITE = 7;

// Not implemented :
// constexpr int32_t UPLOAD_ERR_INI_SIZE = 1;  // unused in kphp
// constexpr int32_t UPLOAD_ERR_FORM_SIZE = 2; // todo support header max-file-size
// constexpr int32_t UPLOAD_ERR_NO_TMP_DIR = 6; // todo support check tmp dir
// constexpr int32_t UPLOAD_ERR_EXTENSION = 8; // unused in kphp

inline constexpr std::string_view LETTERS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
inline constexpr int64_t GENERATE_ATTEMPTS = 4;
inline constexpr int64_t SYMBOLS_COUNT = 6;

char random_letter() noexcept {
  int64_t pos{f$mt_rand(0, LETTERS.size() - 1)};
  return LETTERS[pos];
}

std::optional<kphp::stl::string<kphp::memory::script_allocator>> generate_temporary_name() noexcept {
  // todo rework with k2::tempnam or mkstemp
  auto tmp_dir_env{ComponentState::get().env.get_value(string{"TMPDIR"})};
  std::string_view tmp_path{tmp_dir_env.is_string() ? std::string_view{tmp_dir_env.as_string().c_str(), tmp_dir_env.as_string().size()} : P_tmpdir};
  for (int64_t attempt{}; attempt < GENERATE_ATTEMPTS; ++attempt) {
    kphp::stl::string<kphp::memory::script_allocator> tmp_name{tmp_path.data(), tmp_path.size()};
    tmp_name.push_back('/');
    for (auto _ : std::views::iota(0, SYMBOLS_COUNT)) {
      tmp_name.push_back(random_letter());
    }
    if (!k2::access(tmp_name, F_OK).has_value()) {
      return tmp_name;
    }
  }
  return std::nullopt;
}

std::expected<size_t, int32_t> write_temporary_file(std::string_view tmp_name, std::span<const std::byte> content) noexcept {
  auto file_res{kphp::fs::file::open(tmp_name, "w")};
  if (!file_res.has_value()) {
    return std::unexpected{UPLOAD_ERR_NO_FILE};
  }

  auto written_res{(*file_res).write(content)};
  if (!written_res.has_value()) {
    std::ignore = k2::unlink(tmp_name);
    return std::unexpected{UPLOAD_ERR_CANT_WRITE};
  }

  size_t file_size{*written_res};
  if (file_size < content.size()) {
    std::ignore = k2::unlink(tmp_name);
    return std::unexpected{UPLOAD_ERR_PARTIAL};
  }
  return file_size;
}

array<mixed> build_file_array(std::string_view filename, std::string_view tmp_name, std::string_view content_type,
                              std::expected<size_t, int32_t> write_res) noexcept {
  array<mixed> file{};
  if (!write_res.has_value()) {
    file.set_value(string{"size"}, 0);
    file.set_value(string{"tmp_name"}, string{});
    file.set_value(string{"error"}, write_res.error());
  } else {
    file.set_value(string{"name"}, string{filename.data(), static_cast<string::size_type>(filename.size())});
    file.set_value(string{"type"}, string{content_type.data(), static_cast<string::size_type>(content_type.size())});
    file.set_value(string{"size"}, static_cast<int64_t>(*write_res));
    file.set_value(string{"tmp_name"}, string{tmp_name.data(), static_cast<string::size_type>(tmp_name.size())});
    file.set_value(string{"error"}, UPLOAD_ERR_OK);
  }
  return file;
}

void add_file_to_array(array<mixed>& files, std::string_view name_attribute, array<mixed> file) noexcept {
  if (name_attribute.ends_with("[]")) {
    string name{name_attribute.data(), static_cast<string::size_type>(name_attribute.size() - 2)};
    mixed file_array{files.get_value(name)};
    for (auto& attribute_it : file) {
      string attribute{attribute_it.get_key().to_string()};
      mixed file_array_value{file_array.get_value(attribute)};
      file_array_value.push_back(attribute_it.get_value().to_string());
      file_array.set_value(attribute, file_array_value);
    }
    files.set_value(name, file_array);
  } else {
    string name{name_attribute.data(), static_cast<string::size_type>(name_attribute.size())};
    files.set_value(name, file);
  }
}

} // namespace

namespace kphp::http::multipart::details {

void process_post_multipart(const kphp::http::multipart::details::part& part, array<mixed>& post) noexcept {
  const string name{part.name_attribute.data(), static_cast<string::size_type>(part.name_attribute.size())};
  const string body{part.body.data(), static_cast<string::size_type>(part.body.size())};
  if (part.content_type.has_value() && !std::ranges::search(*part.content_type, CONTENT_TYPE_APP_FORM_URLENCODED).empty()) {
    auto post_value{post.get_value(name)};
    f$parse_str(body, post_value);
    post.set_value(name, std::move(post_value));
  } else {
    post.set_value(name, body);
  }
}

void process_file_multipart(const kphp::http::multipart::details::part& part, array<mixed>& files) noexcept {
  kphp::log::assertion(part.filename_attribute.has_value());
  const auto tmp_name{*(generate_temporary_name().or_else([]() {                  // NOLINT
    kphp::log::error("cannot generate unique name for multipart temporary file"); // no return
    return std::optional<kphp::stl::string<kphp::memory::script_allocator>>{};
  }))};
  const auto body_bytes_span{std::as_bytes(std::span<const char>(part.body.data(), part.body.size()))};
  auto write_res{write_temporary_file(tmp_name, body_bytes_span)};
  if (write_res.has_value()) {
    HttpServerInstanceState::get().multipart_temporary_files.insert(tmp_name);
  }
  const auto content_type{part.content_type.value_or(DEFAULT_CONTENT_TYPE)};
  auto file{build_file_array(*part.filename_attribute, tmp_name, content_type, write_res)};
  add_file_to_array(files, part.name_attribute, std::move(file));
}
} // namespace kphp::http::multipart::details
