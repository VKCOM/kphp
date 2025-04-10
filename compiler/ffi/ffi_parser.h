// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "compiler/ffi/ffi_types.h"

struct FFIParseError {
  std::string message;
  int line;
};

// FFIParseResult contains FFI types extracted from the C header file.
//
// To get these types, iterate over FFIParseResult object with range
// for-loop or use begin()+end() to copy date.
//
// We incapsulate the vector to avoid the need of copying the vector
// while giving `const FFIType*` pointers to the users.
struct FFIParseResult {
private:
  std::vector<FFIType*> types_;

  struct Iter {
    Iter(const std::vector<FFIType*>* vec, int64_t pos)
        : pos_{pos},
          vec_{vec} {}

    bool operator!=(const Iter& other) const noexcept {
      return pos_ != other.pos_;
    }

    const FFIType* operator*() const {
      return (*vec_)[pos_];
    }

    const Iter& operator++() {
      pos_++;
      return *this;
    }

    int64_t pos_;
    const std::vector<FFIType*>* vec_;
  };

public:
  std::string scope;
  std::string lib;
  std::string static_lib;
  std::map<std::string, int> enum_constants;

  FFIParseError err;

  void set_types(std::vector<FFIType*>&& types) {
    types_ = std::move(types);
  }

  size_t num_types() const noexcept {
    return types_.size();
  }
  Iter begin() const noexcept {
    return Iter{&types_, 0};
  }
  Iter end() const noexcept {
    return Iter{&types_, static_cast<int64_t>(types_.size())};
  }
};

FFIParseResult ffi_parse_file(const std::string& src, FFITypedefs& typedefs);

// exposed for testing purposes
std::string ffi_preprocess_file(const std::string& src, FFIParseResult& parse_result);

std::pair<const FFIType*, FFIParseError> ffi_parse_type(const std::string& type_expr, FFITypedefs& typedefs);
