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

template <typename Container,
  typename = decltype(std::declval<Container>().begin()),
  typename = decltype(std::declval<Container>().end())>
std::vector<int> compile_raw_data(CodeGenerator &W, const Container &values) {
  std::string raw_data;
  std::vector<int> const_string_shifts(values.size());
  int ii = 0;
  for (const auto &s : values) {
    int shift_to_align = (((int)raw_data.size() + 7) & -8) - (int)raw_data.size();
    if (shift_to_align != 0) {
      raw_data.append(shift_to_align, 0);
    }
    int raw_len = string_raw_len(static_cast<int>(s.size()));
    kphp_assert (raw_len != -1);
    const_string_shifts[ii] = (int)raw_data.size();
    raw_data.append(raw_len, 0);
    int err = string_raw(&raw_data[const_string_shifts[ii]], raw_len, s.c_str(), (int)s.size());
    kphp_assert (err == raw_len);
    ii++;
  }
  if (!raw_data.empty()) {
    W << "alignas(8) static const char raw[] = " << RawString(raw_data) << ";" << NL;
  }
  return const_string_shifts;
}

inline RawString gen_raw_string(const std::string& data) {
  int raw_len = string_raw_len(data.size());
  std::string response(raw_len, '\0');
  int err = string_raw(response.data(), raw_len, data.c_str(), static_cast<int>(data.size()));
  kphp_assert (err == response.size());
  return RawString(response);
}

