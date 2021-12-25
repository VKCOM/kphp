// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <map>
#include <string>
#include <vector>

#include "compiler/code-gen/code-gen-root-cmd.h"

class CodeGenerator;
class TypeData;
struct IncludesCollector;

struct TypeTagger : CodeGenRootCmd {
  TypeTagger(std::vector<const TypeData *> &&forkable_types, std::vector<const TypeData *> &&waitable_types) noexcept;
  void compile(CodeGenerator &W) const final;

private:
  IncludesCollector collect_includes() const noexcept;
  std::map<int, std::string> collect_hash_of_types() const noexcept;
  void compile_tagger(CodeGenerator &W, const IncludesCollector &includes, const std::map<int, std::string> &hash_of_types) const noexcept;
  void compile_loader_header(CodeGenerator &W, const IncludesCollector &includes, const std::map<int, std::string> &hash_of_types) const noexcept;
  void compile_loader_instantiations(CodeGenerator &W) const noexcept;
  void compile_loader(CodeGenerator &W, const IncludesCollector &includes, const std::map<int, std::string> &hash_of_types) const noexcept;

  constexpr static std::size_t loader_split_granularity_{20};
  const std::string loader_file_{"_loader.h"};
  const std::vector<const TypeData *> forkable_types_;
  const std::vector<const TypeData *> waitable_types_;
};
