// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <set>
#include <unordered_set>
#include <vector>

#include "compiler/data/data_ptr.h"

class VarsCollector {
public:
  explicit VarsCollector(size_t parts, std::function<bool(VarPtr)> vars_checker = {});

  void collect_global_and_static_vars_from(FunctionPtr function);
  std::vector<std::set<VarPtr>> flush();

private:
  template<class It>
  void add_vars(It begin, It end);

  std::unordered_set<FunctionPtr> visited_functions_;
  std::vector<std::set<VarPtr>> collected_vars_;
  std::function<bool(VarPtr)> vars_checker_;
};
