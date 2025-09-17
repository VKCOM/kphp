// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

// ATTENTION!
// This file exists both in KPHP and in a private vkcom repo "ml_experiments".
// They are almost identical, besides include paths and input types (`array` vs `unordered_map`).

#pragma once

#include "common/wrappers/string_view.h"
#include "runtime-common/stdlib/kml/kphp_ml.h"

struct FileReader final : vk::not_copyable {
private:
  // TODO: think about how to store descriptor here for k2 case
  FILE* file_{nullptr};

public:
  explicit FileReader(vk::string_view filename) noexcept
      : file_{fopen(filename.data(), "rb")} {}

  ~FileReader() noexcept {
    if (file_) {
      std::ignore = fclose(file_);
    }
  }

  void read(char* dest, size_t sz) noexcept {
    int read_cnt = fread(dest, 1, sz, file_);
    assert(read_cnt == sz);
  }

  bool is_eof() const noexcept  {
    return file_ ? feof(file_) != 0 : true;
  }
};

std::variant<kphp_ml::MLModel, std::string>  kml_file_read(const std::string& kml_filename);
