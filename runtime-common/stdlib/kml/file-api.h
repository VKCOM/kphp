// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

// ATTENTION!
// This file exists both in KPHP and in a private vkcom repo "ml_experiments".
// They are almost identical, besides include paths and input types (`array` vs `unordered_map`).

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>

#include "runtime-common/core/std/containers.h"

namespace kphp::kml {

inline constexpr std::string_view KML_FILE_EXTENSION = ".kml";

struct file_reader_interface {
  virtual ~file_reader_interface() = default;

  virtual bool is_eof() const noexcept = 0;
  virtual size_t read(void* dest, size_t sz) noexcept = 0;
};

template<class T, class F>
struct dir_traverser_interface {
  virtual ~dir_traverser_interface() = default;

  virtual std::optional<F> next() noexcept = 0;

  static std::optional<T> create(std::string_view dir_path) noexcept;
};

namespace detail {

template<class FileReader, template<class> class Allocator>
class reader {
  FileReader m_reader;

  template<typename T>
  void read_vec_impl(kphp::stl::vector<T, Allocator>& v) noexcept {
    static_assert(std::is_standard_layout_v<T> && std::is_trivial_v<T>);
    int32_t sz = read_int32();
    if (sz == 0) {
      return;
    }

    v.resize(sz);
    read_bytes(v.data(), sz * sizeof(T));
  }

  void read_vec_impl(kphp::stl::vector<kphp::stl::string<Allocator>, Allocator>& v) noexcept {
    int32_t sz = read_int32();
    if (sz == 0) {
      return;
    }

    v.resize(sz);
    for (auto& str : v) {
      read_string(str);
    }
  }

public:
  explicit reader(FileReader reader) noexcept
      : m_reader(std::move(reader)) {}

  int32_t read_int32() noexcept {
    int32_t v = 0;
    m_reader.read(reinterpret_cast<char*>(&v), sizeof(int32_t));
    return v;
  }

  void read_int32(int32_t& v) noexcept {
    m_reader.read(reinterpret_cast<char*>(&v), sizeof(int32_t));
  }

  void read_uint32(uint32_t& v) noexcept {
    m_reader.read(reinterpret_cast<char*>(&v), sizeof(uint32_t));
  }

  void read_uint64(uint64_t& v) noexcept {
    m_reader.read(reinterpret_cast<char*>(&v), sizeof(uint64_t));
  }

  void read_float(float& v) noexcept {
    m_reader.read(reinterpret_cast<char*>(&v), sizeof(float));
  }

  void read_double(double& v) noexcept {
    m_reader.read(reinterpret_cast<char*>(&v), sizeof(double));
  }

  template<class T>
  void read_enum(T& v) noexcept {
    static_assert(sizeof(T) == sizeof(int32_t));
    m_reader.read(reinterpret_cast<char*>(&v), sizeof(int32_t));
  }

  void read_string(kphp::stl::string<Allocator>& v) noexcept {
    int32_t len = 0;
    m_reader.read(reinterpret_cast<char*>(&len), sizeof(int32_t));
    if (len == 0) {
      return;
    }

    v.resize(len);
    m_reader.read(v.data(), len);
  }

  void read_bool(bool& v) noexcept {
    v = static_cast<bool>(read_int32());
  }

  void read_bytes(void* v, size_t len) noexcept {
    m_reader.read(v, len);
  }

  template<class T>
  void read_vec(kphp::stl::vector<T, Allocator>& v) noexcept {
    read_vec_impl(v);
  }

  template<class T>
  void read_2d_vec(kphp::stl::vector<kphp::stl::vector<T, Allocator>, Allocator>& v) noexcept {
    int32_t sz = read_int32();
    if (sz == 0) {
      return;
    }

    v.resize(sz);
    for (auto& elem : v) {
      read_vec(elem);
    }
  }

  [[nodiscard]] bool is_eof() const noexcept {
    return m_reader.is_eof();
  }
};

} // namespace detail

} // namespace kphp::kml
