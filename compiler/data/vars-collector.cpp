// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/data/vars-collector.h"

#include "common/algorithms/hashes.h"

#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"

VarsCollector::VarsCollector(size_t parts, std::function<bool(VarPtr)> vars_checker)
  : collected_vars_(parts)
  , vars_checker_(std::move(vars_checker)) {}

void VarsCollector::collect_global_and_static_vars_from(FunctionPtr function) {
  if (!visited_functions_.emplace(function).second) {
    return;
  }

  for (auto dep_function : function->dep) {
    collect_global_and_static_vars_from(dep_function);
  }

  add_vars(function->global_var_ids.begin(), function->global_var_ids.end());
  add_vars(function->static_var_ids.begin(), function->static_var_ids.end());
}

std::vector<std::set<VarPtr>> VarsCollector::flush() {
  visited_functions_.clear();

  auto last_part = std::remove_if(collected_vars_.begin(), collected_vars_.end(), [](const std::set<VarPtr> &p) { return p.empty(); });
  collected_vars_.erase(last_part, collected_vars_.end());
  return std::move(collected_vars_);
}

template<class It>
void VarsCollector::add_vars(It begin, It end) {
  for (; begin != end; begin++) {
    VarPtr var_id = *begin;
    if (vars_checker_ && !vars_checker_(var_id)) {
      continue;
    }
    const size_t var_hash = var_id->class_id ? vk::std_hash(var_id->class_id->file_id->main_func_name) : vk::std_hash(var_id->name);

    const size_t bucket = var_hash % collected_vars_.size();
    collected_vars_[bucket].emplace(var_id);
  }
}
