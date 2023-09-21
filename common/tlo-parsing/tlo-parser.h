// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstring>
#include <memory>
#include <string>

namespace vk {
namespace tlo_parsing {

struct type_expr_base;
struct nat_expr_base;
struct expr_base;
struct tl_scheme;

struct tlo_parser {
  static constexpr unsigned int MAX_SCHEMA_LEN = 10 * 1024 * 1024;

  tlo_parser() = default;
  explicit tlo_parser(const char *tlo_path);
  ~tlo_parser();

  template<typename T>
  T get_value() {
    check_pos(sizeof(T));
    T res{};
    memcpy(&res, data + pos, sizeof(T));
    pos += sizeof(T);
    return res;
  }

  std::string get_string();

  std::unique_ptr<type_expr_base> read_type_expr();
  std::unique_ptr<nat_expr_base> read_nat_expr();
  std::unique_ptr<expr_base> read_expr();

  void get_schema_version();
  void get_types(bool rename_all_forbidden_names);
  void get_constructors(bool rename_all_forbidden_names);
  void get_functions(bool rename_all_forbidden_names);

  void check_pos(size_t size);
  void error(const char *format, ...) __attribute__ ((format (printf, 2, 3)));

  size_t len;
  size_t pos;
  std::unique_ptr<tl_scheme> tl_sch;
  char data[MAX_SCHEMA_LEN + 1];
};

} // namespace tlo_parsing
} // namespace vk
