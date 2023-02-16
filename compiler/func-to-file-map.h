// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <cassert>
#include <mutex>
#include <string>
#include <unordered_map>
#include <ostream>

class FuncToFileMap {
public:
  FuncToFileMap() = default;
  FuncToFileMap(const FuncToFileMap&) = delete;
  FuncToFileMap& operator=(const FuncToFileMap&) = delete;

  void add_mapping(std::string function_name, std::string file_name) {
    [[maybe_unused]] std::lock_guard guard(mutex);
//    assert(mapping.count(function_name) == 0);
    mapping.insert({std::move(function_name), std::move(file_name)});
  }

  void write_to(std::ostream &out) const {
    [[maybe_unused]] std::lock_guard guard(mutex);
    out << "{\n";
    for (const auto& func_to_file : mapping) {
      out << "\"" << func_to_file.first << "\": \"" << func_to_file.second << "\",\n";
    }
    out << "}\n";
  }

private:
  mutable std::mutex mutex;
  std::unordered_map<std::string, std::string> mapping;
};