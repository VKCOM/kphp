// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "php-classes.h"

namespace vk {
namespace tl {

struct TlHint {
  std::string magic;
  std::vector<std::string> args;
  std::string result;
};

class TlHints {
public:
  void load_from_combined2_tl_file(const std::string& combined2_tl_file, bool rename_all_forbidden_names = true);

  const TlHint* get_hint_for_combinator(const std::string& combinator_name) const;

private:
  std::unordered_map<std::string, TlHint> hints_;
};

} // namespace tl
} // namespace vk
