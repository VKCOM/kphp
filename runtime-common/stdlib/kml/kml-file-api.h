// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

// ATTENTION!
// This file exists both in KPHP and in a private vkcom repo "ml_experiments".
// They are almost identical, besides include paths and input types (`array` vs `unordered_map`).

#pragma once

#include <dirent.h>
#include <sys/stat.h>

#include "common/wrappers/string_view.h"
#include "runtime-common/stdlib/kml/kphp_ml.h"
#include "runtime-common/stdlib/kml/kphp_ml_stl.h"

namespace kphp_ml {
struct IFileReader {
  virtual ~IFileReader() = default;
  virtual void read(char* dest, size_t sz) noexcept = 0;
  virtual bool is_eof() const noexcept = 0;
};

struct IDirTraverser {
  using callback_t = void (*)(const kphp_ml::stl::string& fname);

  virtual ~IDirTraverser() = default;
  virtual void traverse(const kphp_ml::stl::string&) = 0;
};

std::variant<kphp_ml::MLModel, kphp_ml::stl::string> kml_file_read(const kphp_ml::stl::string& kml_filename);

// Definitions are in runtime and runtime-light dirs
kphp_ml::stl::unique_ptr_platform<kphp_ml::IFileReader> get_file_reader(vk::string_view file_name);
kphp_ml::stl::unique_ptr_platform<IDirTraverser> get_dir_traverser(IDirTraverser::callback_t callback);

} // namespace kphp_ml
