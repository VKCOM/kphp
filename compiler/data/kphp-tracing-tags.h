// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>
#include <forward_list>

#include "compiler/data/data_ptr.h"
#include "common/wrappers/string_view.h"

class KphpTracingDeclarationMixin {
  static std::string generate_default_span_title(FunctionPtr f);

  static int parse_level_attr(vk::string_view beg, size_t &pos_end);
  static std::string parse_string_attr(vk::string_view beg, size_t &pos_end);
  static std::pair<int, std::string> parse_branch_attr(vk::string_view beg, size_t &pos_end);

public:
  int level{1};
  std::string span_title;
  std::string aggregate_name;
  std::forward_list<std::pair<int, std::string>> branches;

  bool is_aggregate() const { return !aggregate_name.empty(); }

  static KphpTracingDeclarationMixin *create_for_function_from_phpdoc(FunctionPtr f, vk::string_view value);
  static KphpTracingDeclarationMixin *create_for_shutdown_function(FunctionPtr f);
};
