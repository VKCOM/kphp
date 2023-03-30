// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>
#include <utility>

#include "common/php-functions.h"
#include "compiler/code-gen/code-generator.h"
#include "compiler/code-gen/common.h"
#include "compiler/vertex.h"

class RawString {
public:
  explicit RawString(vk::string_view str) :
    str(str) {
  }

  void compile(CodeGenerator &W) const;

private:
  std::string str;
};

std::vector<int> compile_arrays_raw_representation(const std::vector<VarPtr> &const_raw_array_vars, CodeGenerator &W);

inline RawString gen_raw_string(const std::string& data) {
  int raw_len = string_raw_len(data.size());
  std::string response(raw_len, '\0');
  int err = string_raw(response.data(), raw_len, data.c_str(), static_cast<int>(data.size()));
  kphp_assert (err == response.size());
  return RawString(response);
}

