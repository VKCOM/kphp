// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>
#include <sys/stat.h>
#include <utility>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/stdlib/kml/file-api.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/file/resource.h"

namespace kphp::kml {

namespace detail {

template<template<class> class Allocator>
class file_reader final : public kphp::kml::file_reader_interface {
  kphp::fs::file m_file;
  kphp::stl::string<Allocator> m_path;

public:
  file_reader(kphp::fs::file file, kphp::stl::string<Allocator> path) noexcept
      : m_file(std::move(file)),
        m_path(std::move(path)) {}

  file_reader(const file_reader&) = delete;
  file_reader(file_reader&& other) noexcept
      : m_file(std::move(other.m_file)),
        m_path(std::move(other.m_path)) {}

  file_reader& operator=(const file_reader&) = delete;
  file_reader& operator=(file_reader&& other) noexcept {
    if (this != std::addressof(other)) {
      m_file = std::move(other.m_file);
      m_path = std::move(other.m_path);
    }
    return *this;
  }

  ~file_reader() final = default;

  size_t read(void* dest, size_t sz) noexcept final {
    if (dest == nullptr) [[unlikely]] {
      kphp::log::error("[kml] failed to read into nullptr: requested size -> {}", sz);
    }

    auto expected{m_file.read({reinterpret_cast<std::byte*>(dest), sz})};
    if (!expected) [[unlikely]] {
      kphp::log::warning("[kml] failed to read {} bytes: error code -> {}, path -> {}", sz, expected.error(), m_path);
      return 0;
    }
    return *expected;
  }

  bool is_eof() const noexcept final {
    auto expected{m_file.eof()};
    if (!expected) [[unlikely]] {
      kphp::log::warning("[kml] failed to check EOF for file: error code -> {}, path -> {}", expected.error(), m_path);
      return true;
    }
    return *expected;
  }
};

template<template<class> class Allocator>
class dir_traverser final : public kphp::kml::dir_traverser_interface<dir_traverser<Allocator>, file_reader<Allocator>> {
  kphp::stl::stack<std::pair<kphp::fs::directory, kphp::stl::string<Allocator>>, Allocator> m_dirs;

public:
  dir_traverser(kphp::fs::directory dir, kphp::stl::string<Allocator> path) noexcept {
    m_dirs.emplace(std::move(dir), std::move(path));
  }

  dir_traverser(const dir_traverser&) = delete;
  dir_traverser(dir_traverser&& other) noexcept
      : m_dirs(std::move(other.m_dirs)) {}

  dir_traverser& operator=(const dir_traverser&) = delete;
  dir_traverser& operator=(dir_traverser&& other) noexcept {
    if (this != std::addressof(other)) {
      m_dirs = std::move(other.m_dirs);
    }
    return *this;
  }

  ~dir_traverser() final = default;

  std::optional<file_reader<Allocator>> next() noexcept final {
    if (m_dirs.empty()) {
      return std::nullopt;
    }

    const auto& [cur_dir, cur_path]{m_dirs.top()};
    auto opt_direntry{cur_dir.readdir()};
    if (!opt_direntry) { // we reached the end of the directory
      m_dirs.pop();
      return next();
    }

    auto expected_direntry{*std::move(opt_direntry)};
    if (!expected_direntry) [[unlikely]] {
      kphp::log::warning("[kml] failed to iterate over directory: error code -> {}, path -> {}", expected_direntry.error(), cur_path);
      return std::nullopt;
    }

    const auto& direntry{*expected_direntry};
    const std::string_view direntry_filename{direntry.filename.first.get(), direntry.filename.second};
    const std::string_view direntry_path{direntry.path.first.get(), direntry.path.second};
    if (direntry_filename == "." || direntry_filename == "..") {
      return next();
    }

    struct stat stat {};
    if (auto error_code_expected{k2::stat(direntry_path, std::addressof(stat))}; !error_code_expected.has_value()) [[unlikely]] {
      kphp::log::warning("[kml] failed to get stat: error code -> {}, path -> {}", error_code_expected.error(), direntry_path);
      return std::nullopt;
    }

    const auto is_directory{S_ISDIR(stat.st_mode)};
    if (!is_directory && direntry_filename.ends_with(kphp::kml::KML_FILE_EXTENSION)) {
      auto expected_file{kphp::fs::file::open(direntry_path, "r")};
      if (!expected_file) [[unlikely]] {
        kphp::log::warning("[kml] failed to open a file: error code -> {}, path -> {}", expected_file.error(), direntry_path);
        return std::nullopt;
      }

      kphp::log::info("[kml] opening a file: path -> {}", direntry_path);
      return file_reader{*std::move(expected_file), kphp::stl::string<Allocator>{direntry_path}};
    }

    if (is_directory) {
      auto expected_dir{kphp::fs::directory::open(direntry_path)};
      if (!expected_dir) [[unlikely]] {
        kphp::log::warning("[kml] failed to open a directory: error code -> {}, path -> {}", expected_dir.error(), direntry_path);
        return std::nullopt;
      }

      kphp::log::info("[kml] opening a directory: path -> {}", direntry_path);
      m_dirs.emplace(*std::move(expected_dir), direntry_path);
    }
    return next();
  }
};

} // namespace detail

template<>
inline std::optional<detail::dir_traverser<kphp::memory::script_allocator>>
kphp::kml::dir_traverser_interface<detail::dir_traverser<kphp::memory::script_allocator>, detail::file_reader<kphp::memory::script_allocator>>::create(
    std::string_view path) noexcept {
  auto expected_dir{kphp::fs::directory::open(path)};
  if (!expected_dir) [[unlikely]] {
    kphp::log::warning("[kml] failed to open a directory: error code -> {}, path -> {}", expected_dir.error(), path);
    return std::nullopt;
  }

  kphp::log::info("[kml] opening a directory: path -> {}", path);
  return detail::dir_traverser<kphp::memory::script_allocator>{*std::move(expected_dir), kphp::stl::string<kphp::memory::script_allocator>{path}};
}

} // namespace kphp::kml
