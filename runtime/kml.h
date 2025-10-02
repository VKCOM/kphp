// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <memory>
#include <optional>
#include <stack>
#include <string_view>
#include <sys/stat.h>
#include <tuple>
#include <utility>

#include "runtime-common/core/allocator/platform-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-common/stdlib/kml/file-api.h"
#include "runtime-common/stdlib/kml/inference-context.h"
#include "runtime-common/stdlib/kml/models-context.h"

template<>
struct std::hash<kphp::stl::string<kphp::memory::platform_allocator>> {
  size_t operator()(const kphp::stl::string<kphp::memory::platform_allocator>& s) const noexcept {
    std::string_view view{s.c_str(), s.size()};
    return std::hash<std::string_view>{}(view);
  }
};

namespace kphp::kml {

extern const char* kml_dir;

namespace detail {

class file_reader final : public kphp::kml::file_reader_interface {
  std::FILE* m_file{};
  kphp::stl::string<kphp::memory::platform_allocator> m_path;

  void close() noexcept {
    if (m_file) {
      std::ignore = std::fclose(std::exchange(m_file, nullptr));
    }
  }

public:
  file_reader(std::FILE* file, kphp::stl::string<kphp::memory::platform_allocator> path) noexcept
      : m_file(file),
        m_path(std::move(path)) {}

  file_reader(const file_reader&) = delete;
  file_reader(file_reader&& other) noexcept
      : m_file(std::exchange(other.m_file, nullptr)),
        m_path(std::move(other.m_path)) {}

  file_reader& operator=(const file_reader&) = delete;
  file_reader& operator=(file_reader&& other) noexcept {
    if (this != std::addressof(other)) {
      close();
      m_file = std::exchange(other.m_file, nullptr);
      m_path = std::move(other.m_path);
    }
    return *this;
  }

  ~file_reader() final {
    close();
  }

  size_t read(void* dest, size_t sz) noexcept final {
    if (m_file == nullptr) {
      php_warning("[kml] failed to read from NULL: path -> %s", m_path.c_str());
      return 0;
    }

    if (dest == nullptr) {
      php_critical_error("[kml] failed to read into NULL: requested size -> %zu", sz);
    }

    const auto read{std::fread(dest, 1, sz, m_file)};
    if (read != sz) {
      php_warning("[kml] failed to read %zu bytes: read -> %zu, path -> %s", sz, read, m_path.c_str());
    }
    return read;
  }

  bool is_eof() const noexcept final {
    if (m_file == nullptr) {
      php_warning("[kml] failed to check for EOF on NULL: path -> %s", m_path.c_str());
      return true;
    }
    return std::feof(m_file) != 0;
  }

  static std::optional<file_reader> create(kphp::stl::string<kphp::memory::platform_allocator> path) noexcept {
    php_info("[kml] opening the file -> %s", path.c_str());
    auto* file = std::fopen(path.c_str(), "rb");
    if (file == nullptr) {
      php_warning("[kml] failed to open the file -> %s", path.c_str());
      return std::nullopt;
    }
    return kphp::kml::detail::file_reader{file, std::move(path)};
  }
};

} // namespace detail

namespace detail {

inline bool ends_with(const char* str, const char* suffix) noexcept {
  size_t len_str = strlen(str);
  size_t len_suffix = strlen(suffix);

  return len_suffix <= len_str && strncmp(str + len_str - len_suffix, suffix, len_suffix) == 0;
}

class dir_traverser final : public kphp::kml::dir_traverser_interface<dir_traverser, file_reader> {
  std::stack<std::pair<DIR*, kphp::stl::string<kphp::memory::platform_allocator>>> m_dirs;

  void close() noexcept {
    while (!m_dirs.empty()) {
      auto& [dir, _]{m_dirs.top()};
      std::ignore = closedir(dir);
      m_dirs.pop();
    }
  }

public:
  explicit dir_traverser(DIR* dir, std::string_view path) noexcept {
    if (dir != nullptr) {
      m_dirs.emplace(dir, path);
    }
  }

  dir_traverser(const dir_traverser&) = delete;
  dir_traverser(dir_traverser&& other) noexcept
      : m_dirs(std::move(other.m_dirs)) {}

  dir_traverser& operator=(const dir_traverser&) = delete;
  dir_traverser& operator=(dir_traverser&& other) noexcept {
    if (this != std::addressof(other)) {
      close();
      m_dirs = std::move(other.m_dirs);
    }
    return *this;
  }

  ~dir_traverser() final {
    close();
  }

  std::optional<file_reader> next() noexcept final {
    if (m_dirs.empty()) {
      return std::nullopt;
    }

    auto& [dir, path]{m_dirs.top()};
    auto* direntry{readdir(dir)};
    if (direntry == nullptr) { // we reached the end of the directory
      closedir(dir);
      m_dirs.pop();
      return next();
    }

    if (std::strcmp(direntry->d_name, ".") == 0 || std::strcmp(direntry->d_name, "..") == 0) {
      return next();
    }

    kphp::stl::string<kphp::memory::platform_allocator> new_path{path + "/" + direntry->d_name};

    struct stat direntry_stat {};
    if (stat(new_path.c_str(), std::addressof(direntry_stat)) != 0) {
      php_warning("[kml] failed to get stat: path -> %s", new_path.c_str());
      return std::nullopt;
    }

    const auto is_directory{S_ISDIR(direntry_stat.st_mode)};
    if (!is_directory && ends_with(direntry->d_name, kphp::kml::KML_FILE_EXTENSION.data())) {
      return file_reader::create(std::move(new_path));
    }

    if (is_directory) {
      if (auto* next_dir{opendir(new_path.c_str())}; next_dir != nullptr) {
        m_dirs.emplace(next_dir, std::move(new_path));
      } else {
        php_warning("[kml] failed to open subdirectory: path -> %s", new_path.c_str());
      }
    }
    return next();
  }
};

} // namespace detail

template<>
inline std::optional<detail::dir_traverser>
kphp::kml::dir_traverser_interface<detail::dir_traverser, detail::file_reader>::create(std::string_view dir_path) noexcept {
  php_info("[kml] opening the directory -> %s", dir_path.data());
  auto* dir{opendir(dir_path.data())};
  if (dir == nullptr) {
    php_warning("[kml] failed to open the directory: directory -> %s", dir_path.data());
    return std::nullopt;
  }
  return kphp::kml::detail::dir_traverser{dir, dir_path};
}

} // namespace kphp::kml

using KmlModelsState = KmlModelsContext<kphp::kml::detail::dir_traverser, kphp::memory::platform_allocator>;
using KmlInferenceState = KmlInferenceContext<kphp::memory::platform_allocator>;
